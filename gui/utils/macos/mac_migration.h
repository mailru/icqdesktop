//
//  mac_migration.h
//  ICQ
//
//  Created by Vladimir Kubyshev on 25/02/16.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

class MacProfile final
{
public:
    enum class Type
    {
        ICQ     = 0,
        Agent,
    };

public:
    MacProfile(const Type &type, const QString &identifier, const QString &uin, const QString &pw = "");
    ~MacProfile();
    
    void setName(const QString &name);
    void setToken(const QString &token);
    void setAimsid(const QString &aimsid);
    void setKey(const QString &key);
    void setFetchUrl(const QString &fetchUrl);
    void setTimeOffset(time_t timeOffset);
    
    inline const Type &type() const { return type_; }
    const QString &name() const;
    const QString &uin() const;
    const QString &pw() const;
    const QString &identifier() const;
    const QString &token() const;
    const QString &aimsid() const;
    const QString &key() const;
    const QString &fetchUrl() const;
    time_t timeOffset() const;
    
private:
    Type type_;
    QString uin_;
    QString pw_;
    QString name_;
    QString token_;
    QString key_;
    QString aimsid_;
    QString identifier_;
    QString fetchUrl_;
    time_t timeOffset_;
};

typedef QList<MacProfile> MacProfilesList;

class MacMigrationManager
{
public:
    MacMigrationManager(QString accountId);
    virtual ~MacMigrationManager();
    
    inline const MacProfilesList &getProfiles() const { return profiles_; }
    bool migrateProfile(const MacProfile &profile);
    bool mergeProfiles(const MacProfile &profile1, const MacProfile &profile2);
    
    static MacProfilesList profiles1(QString profilesPath, QString generalPath);
    static MacProfilesList profiles2(QString accountsDirectory, QString account);
    static QString canMigrateAccount();
    
private:
    QString accountId_;
    MacProfilesList profiles_;
};


