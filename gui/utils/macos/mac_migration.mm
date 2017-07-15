//
//  mac_migration.m
//  ICQ
//
//  Created by Vladimir Kubyshev on 25/02/16.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <Sparkle/Sparkle.h>
#import <HockeySDK/HockeySDK.h>

#import <Quartz/Quartz.h>

#include "stdafx.h"

#import "mac_migration.h"
#import "mac_support.h"

#include <objc/objc.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include "qmpreferences.h"
#include "../../../common.shared/constants.h"
#include "../gui_coll_helper.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#import "SSKeychain.h"
#import "NSData+Base64.h"

namespace
{
    static QString toQString(NSString * src)
    {
        return QString::fromCFString((__bridge CFStringRef)src);
    }
    
    static NSString * fromQString(const QString & src)
    {
        return (NSString *)CFBridgingRelease(src.toCFString());
    }
    
    QStringList plistAccounts(QString accountsPath)
    {
        QStringList list;
        
        NSArray * accounts = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:fromQString(accountsPath) error:nil];
        
        for (NSString * account in accounts)
        {
            list.append(toQString(account));
        }
        
        return list;
    }
    
    bool checkExists(QString path, bool dir)
    {
        QFileInfo checkFile(path);
        
        if (!checkFile.exists())
        {
            return false;
        }
        
        if ((dir && !checkFile.isDir()) ||
            (!dir && !checkFile.isFile()))
        {
            return false;
        }
        
        return true;
    }
    
    bool checkAccountDirectory(QString accountDirectory, QString identifier)
    {
        if (!checkExists(accountDirectory, true))
        {
            return false;
        }
        
        return MacMigrationManager::profiles2(accountDirectory, identifier).size() > 0;
    }
    
    bool createProtolibProfile(QString protoSettings, QString profileId, QString uin, bool isMain, MacProfilesList & list)
    {
        QMPreferences prefs(protoSettings);
        
        if (prefs.load())
        {
            QString aimsid = prefs.get<QString>("wim.session.aimsid");
            QString displayId = prefs.get<QString>("wim.session.displayId");
            QString fetchUrl = prefs.get<QString>("wim.session.fetchUrl");
            time_t localTimeSkew = prefs.get<int>("wim.localTimeSkew");
            QString authToken = prefs.get<QString>("wim.session.authToken");
            QString sessionKey = prefs.get<QString>("wim.session.sessionKey");
            
            MacProfile newProfile(MacProfile::Type::ICQ, profileId, uin);
            
            newProfile.setName(displayId);
            newProfile.setToken(authToken);
            newProfile.setKey(sessionKey);
            newProfile.setAimsid(aimsid);
            newProfile.setFetchUrl(fetchUrl);
            newProfile.setTimeOffset(localTimeSkew);
            
            if (isMain)
            {
                list.insert(0, newProfile);
            }
            else
            {
                list.append(newProfile);
            }
            
            return true;
        }
        
        return false;
    }
}

MacProfile::MacProfile(const Type &type, const QString &identifier, const QString &uin, const QString &pw):
    type_(type),
    identifier_(identifier),
    uin_(uin),
    pw_(pw)
{
}

MacProfile::~MacProfile()
{
}

void MacProfile::setName(const QString &name)
{
    name_ = name;
}

const QString & MacProfile::name() const
{
    return name_;
}

void MacProfile::setToken(const QString &token)
{
    token_ = token;
}

void MacProfile::setAimsid(const QString &aimsid)
{
    aimsid_ = aimsid;
}

void MacProfile::setKey(const QString &key)
{
    key_ = key;
}

void MacProfile::setFetchUrl(const QString &fetchUrl)
{
    fetchUrl_ = fetchUrl;
}

void MacProfile::setTimeOffset(time_t timeOffset)
{
    timeOffset_ = timeOffset;
}

const QString &MacProfile::uin() const
{
    return uin_;
}

const QString &MacProfile::pw() const
{
    return pw_;
}

const QString &MacProfile::identifier() const
{
    return identifier_;
}

const QString &MacProfile::token() const
{
    return token_;
}

const QString &MacProfile::aimsid() const
{
    return aimsid_;
}

const QString &MacProfile::key() const
{
    return key_;
}

const QString &MacProfile::fetchUrl() const
{
    return fetchUrl_;
}

time_t MacProfile::timeOffset() const
{
    return timeOffset_;
}

MacMigrationManager::MacMigrationManager(QString accountId):
    accountId_(accountId)
{
    if (accountId == "account")
    {
        QString profilesPath = MacSupport::settingsPath().append("/settings/profiles.dat");
        QString generalPath = MacSupport::settingsPath().append("/settings/general.dat");
        
        profiles_ = MacMigrationManager::profiles1(profilesPath, generalPath);
    }
    else
    {
        QString accountsDirectory = MacSupport::settingsPath();
        accountsDirectory = accountsDirectory.append("/Accounts").append("/").append(accountId);
    
        profiles_ = MacMigrationManager::profiles2(accountsDirectory, accountId);
    }
}

MacMigrationManager::~MacMigrationManager()
{
}

MacProfilesList MacMigrationManager::profiles1(QString profilesPath, QString generalPath)
{
    NSData * dataProfiles = [NSData dataWithContentsOfFile:fromQString(profilesPath)];
    NSData * dataGenerals = [NSData dataWithContentsOfFile:fromQString(generalPath)];
    
    MacProfilesList list;
    
    NSDictionary * profiles = dataProfiles.length?
        [NSPropertyListSerialization propertyListWithData:dataProfiles options:0 format:0 error:nil]:
        nil;
    
    NSDictionary * generals = dataGenerals.length?
        [NSPropertyListSerialization propertyListWithData:dataGenerals options:0 format:0 error:nil]:
        nil;
    
    if (profiles.count && generals.count)
    {
        NSString * account = generals[@"account"][@"account_id"];
        
        for (NSDictionary * profile in profiles.allValues)
        {
            QString profileId = toQString(profile[@"key"]);
            
            QString uin = toQString(profile[@"uid"]);
            BOOL isMain = [profile[@"order"] integerValue] == 0;
            NSString * proto = profile[@"protocol"];

            if ([proto isEqualToString:@"wim"] || [proto isEqualToString:@"phone-icq"])
            {
                QString aimsid;
                QString displayId;
                QString fetchUrl;
                time_t localTimeSkew = 0;
                QString authToken;
                QString sessionKey;
                
                NSDictionary * params = generals[[NSString stringWithFormat:@"profile_%@", profile[@"key"]]];
                if (params.count)
                {
                    aimsid = toQString(params[@"aims-id"]);
                    displayId = toQString(params[@"display-id"]);
                    if (displayId.isEmpty())
                    {
                        displayId = toQString(params[@"nickname"]);
                    }
                    if (displayId.isEmpty())
                    {
                        displayId = uin;
                    }
                    fetchUrl = toQString(params[@"fetch-url"]);
                    localTimeSkew = [params[@"time-skew"] intValue];
                }
                
                NSString * keychainAcc = [NSString stringWithFormat:@"%@#%@", account.length?account:profile[@"key"], profile[@"key"]];
                
                NSString * keychainPass = [SSKeychain passwordForService:[NSString stringWithUTF8String:(build::is_icq() ? product_name_icq_mac_a : product_name_agent_mac_a)] account:keychainAcc];
                
                NSRange delimiter = [keychainPass rangeOfString:@"-"];
                if (delimiter.location != NSNotFound)
                {
                    NSString *sessionKeyString = [keychainPass substringToIndex:delimiter.location];
                    NSString *authTokenString = [keychainPass substringFromIndex:delimiter.location + 1];

                    //check for keychain record is token, not password with "-"
                    if (keychainPass.length > 10 &&
                        authTokenString.length > 10 &&
                        sessionKeyString.length > 10)
                    {
                        NSData * d = [NSData dataWithBase64EncodedString:authTokenString];
                        authToken = toQString([[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]);
                        
                        d = [NSData dataWithBase64EncodedString:sessionKeyString];
                        sessionKey = toQString([[NSString alloc] initWithData:d encoding:NSUTF8StringEncoding]);
                    }
                }
                
                if (authToken.isEmpty() && keychainPass.length > 0)
                {
                    authToken = toQString(keychainPass);
                }
                
                MacProfile newProfile(MacProfile::Type::ICQ, profileId, uin);

                newProfile.setName(displayId);
                newProfile.setToken(authToken);
                newProfile.setKey(sessionKey);
                newProfile.setAimsid(aimsid);
                newProfile.setFetchUrl(fetchUrl);
                newProfile.setTimeOffset(localTimeSkew);
                
                if (isMain)
                {
                    list.insert(0, newProfile);
                }
                else
                {
                    list.append(newProfile);
                }
            }
            else if ([proto isEqualToString:@"mmp"])
            {
                QString displayId;
                time_t localTimeSkew = 0;
                
                NSDictionary * params = generals[[NSString stringWithFormat:@"profile_%@", profile[@"key"]]];

                if (params.count)
                {
                    displayId = toQString(params[@"nickname"]);
                    localTimeSkew = [params[@"time-skew"] intValue];
                }
                
                NSString * keychainAcc = [NSString stringWithFormat:@"%@#%@", account.length?account:profile[@"key"], profile[@"key"]];
                
                NSString * keychainPass = [SSKeychain passwordForService:[NSString stringWithUTF8String:(build::is_icq() ? product_name_icq_mac_a : product_name_agent_mac_a)] account:keychainAcc];
                
                MacProfile newProfile(MacProfile::Type::Agent, profileId, uin, toQString(keychainPass));
                newProfile.setName(displayId);
                newProfile.setTimeOffset(localTimeSkew);
                
                if (isMain)
                {
                    list.insert(0, newProfile);
                }
                else
                {
                    list.append(newProfile);
                }
            }
        }
    }
    
    return list;
}

MacProfilesList MacMigrationManager::profiles2(QString accountDirectory, QString account)
{
    QString settingsPath = accountDirectory.append("/").append("Settings.plist");
    
    NSData * data = [NSData dataWithContentsOfFile:fromQString(settingsPath)];
    
    MacProfilesList list;
    
    if (data.length)
    {
        NSDictionary * json = [NSPropertyListSerialization propertyListWithData:data options:0 format:0 error:nil];
        
        if (json.count)
        {
            QString identifier = toQString(json[@"identifier"]);
            if (account == identifier)
            {
                NSArray * profiles = json[@"profiles"];
                
                for (NSDictionary * profile in profiles)
                {
                    QString profileId = toQString(profile[@"identifier"]);
                    QString uin = toQString(profile[@"uid"]);
                    BOOL isMain = [profile[@"is-main"] boolValue];
                    NSString * proto = profile[@"protocol"];
                    
                    if ([proto isEqualToString:@"wim"] ||
                        [proto isEqualToString:@"phone-icq"])
                    {
                        QString settings = MacSupport::settingsPath();
                        settings = settings.append("/").append(uin).append("_wim.dat");
                        
                        if (!createProtolibProfile(settings, profileId, uin, isMain, list))
                        {
                            settings.append(".old");
                            
                            createProtolibProfile(settings, profileId, uin, isMain, list);
                        }
                    }
                }
            }
        }
    }
    
    return list;
}

bool MacMigrationManager::migrateProfile(const MacProfile &profile)
{
    Ui::get_gui_settings()->set_value<bool>(settings_mac_accounts_migrated, true);
    
    if (profile.key().isEmpty())
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("login", profile.uin());
        if (profile.type() == MacProfile::Type::ICQ)
        {
            collection.set_value_as_qstring("password", profile.token());
        }
        else if (profile.type() == MacProfile::Type::Agent)
        {
            collection.set_value_as_qstring("password", profile.pw());
        }
        collection.set_value_as_bool("save_auth_data", true);
        collection.set_value_as_bool("is_login", true);
        collection.set_value_as_bool("not_log", true);
        Ui::GetDispatcher()->post_message_to_core("login_by_password", collection.get());
        
        return true;
    }
    
    NSMutableDictionary * json = [NSMutableDictionary dictionary];

    json[@"login"] = fromQString(profile.uin());
    json[@"aimid"] = fromQString(profile.uin());
    json[@"timeoffset"] = @(profile.timeOffset());
    json[@"devid"] = @"ic18eTwFBO7vAdt9";
    if (profile.type() == MacProfile::Type::ICQ)
    {
        json[@"atoken"] = fromQString(profile.token());
        json[@"sessionkey"] = fromQString(profile.key());
        json[@"aimsid"] = fromQString(profile.aimsid());
        json[@"fetchurl"] = fromQString(profile.fetchUrl());
    }
    else if (profile.type() == MacProfile::Type::Agent)
    {
        //
    }
    
    NSData * data = [NSJSONSerialization dataWithJSONObject:json options:NSJSONWritingPrettyPrinted error:nil];
    
    if (data.length)
    {
        QString settingsPath = MacSupport::settingsPath();
        
        settingsPath = settingsPath.append("/0001/key/");
        
        NSString * fileName = fromQString(settingsPath);
        [[NSFileManager defaultManager] createDirectoryAtPath:fileName withIntermediateDirectories:YES attributes:nil error:nil];
        
        settingsPath = settingsPath.append(auth_export_file_name);
        
        fileName = fromQString(settingsPath);
        if ([data writeToFile:fileName atomically:YES])
        {
            Ui::GetDispatcher()->post_message_to_core("connect_after_migration", nullptr);
            
            return true;
        }
    }

    return false;
}

bool MacMigrationManager::mergeProfiles(const MacProfile &profile1, const MacProfile &profile2)
{
    NSLog(@"%s %s", profile1.uin().toStdString().c_str(), profile2.uin().toStdString().c_str());
    return false;
}

QString MacMigrationManager::canMigrateAccount()
{
    //check new account settings (1.3.9, 2.x)
    
    QString checkPath = MacSupport::settingsPath().append("/settings/profiles.dat");
    
    if (checkExists(checkPath, false))
    {
        QString generalPath = MacSupport::settingsPath().append("/settings/general.dat");
        
        if (MacMigrationManager::profiles1(checkPath, generalPath).size() > 0)
        {
            return "account";
        }
    }
    
    //check old account settings (1.3.x)
    
    checkPath = MacSupport::settingsPath().append("/Accounts");
    
    if (!checkExists(checkPath, true))
    {
        return "";
    }

    QStringList accounts = plistAccounts(checkPath);
    
    for (int i = 0; i < accounts.size(); i++)
    {
        QString account = accounts[i];
        
        QString accPath = checkPath;
        accPath.append("/").append(account);
        
        if (checkAccountDirectory(accPath, account))
        {
            return account;
        }
    }
    
    return "";
}

