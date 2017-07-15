from __future__ import print_function
if __name__ != '__main__':
	sys.exit(0)

import os, sys, shutil
import fileinput
import distutils.dir_util
import requests
import zipfile
from os.path import basename
from subprocess import Popen

DETACHED_PROCESS = 0x00000008

def get_str_version_number(build_number):	
	ICQ_VERSION_MAJOR = '10'
	ICQ_VERSION_MINOR = '0'
	return ".".join((ICQ_VERSION_MAJOR, ICQ_VERSION_MINOR, build_number))
	
def update_version(build_number):
	prod_version_str_macro = "#define VERSION_INFO_STR"
	file_version_str_macro = "#define VER_FILEVERSION_STR"
	file_version_macro = "#define VER_FILEVERSION"

	STR_VERSION = get_str_version_number(build_number)
	VERSION = STR_VERSION.replace('.', ',')

	for filename in ("../common.shared/version_info_constants.h", ):
		for line in fileinput.input(filename, inplace=True):
			if prod_version_str_macro in line:
				line = prod_version_str_macro + ' "'+STR_VERSION+'"\n'
			elif file_version_str_macro in line:
				line = file_version_str_macro + ' "'+STR_VERSION+'"\n'
			elif file_version_macro in line:
				line = file_version_macro + ' '+VERSION+'\n'
			print(line, end='')

icq_app_type = "ICQ_APP"
agent_app_type = "AGENT_APP"

final_build_string = "ICQ_FINAL_BUILD"
development_build_string = "ICQ_DEVELOPMENT_BUILD"

def print_usage():
	app_type_option = "[{0}|{1}]".format(icq_app_type, agent_app_type)
	build_option = "[{0}|{1}]".format(final_build_string, development_build_string)
	print("usage: buildcmake.py " + app_type_option + " " + build_option)
	exit()

def log_and_exec(code):
	print("--> " + code)
	exec(code)

def log_and_eval(code):
	print("--> " + code)
	return eval(code)

if len(sys.argv) != 3:
	print_usage()

is_icq = False

if sys.argv[1] == icq_app_type:
	is_icq = True
elif sys.argv[1] == agent_app_type:
	is_icq = False
else:
	print_usage()

is_final_build = False

if sys.argv[2] == final_build_string:
	is_final_build = True
elif sys.argv[2] == development_build_string:
	is_final_build = False
else:
	print_usage()
	
build_path = os.getenv('MSBUILD', 'C:/Program Files (x86)/MSBuild/14.0/Bin/')
build_path = '"' + build_path + '/MSBuild.exe"'

build_number = os.getenv('BUILD_NUMBER', '1999')

update_version(build_number)

if log_and_eval('os.system("buildcmake.bat {0} {1}".format(("ICQ" if is_icq else "Agent"), (final_build_string if is_final_build else development_build_string)))') != 0:
	sys.exit(1)

def sign_file(file_name):
	if (is_icq):
		os.system("signtool.exe sign /t http://timestamp.verisign.com/scripts/timstamp.dll  /v " + file_name)
		os.system("signtool.exe sign /fd sha256 /tr http://timestamp.globalsign.com/?signature=sha2  /td sha256 /as /v " + file_name)
	else:
		execute_command = "curl.exe -F file=@" + file_name + " http://sign.corp.mail.ru/sign_agent_sha1 -o " + file_name + ".tmp"
		print(execute_command)
		os.system(execute_command)
		shutil.move(file_name + ".tmp", file_name)


log_and_exec('sign_file(os.path.abspath("../bin/Release/icq.exe"))')
log_and_exec('sign_file(os.path.abspath("../bin/Release/corelib.dll"))')
log_and_exec('sign_file(os.path.abspath("../bin/Release/libvoip_x86.dll"))')


if is_icq:
	if log_and_eval('os.system(build_path + " ../installer/installer_icq.sln /t:Rebuild /p:Configuration=Release;Platform=Win32")') != 0:
		sys.exit(1)	
else:
	if log_and_eval('os.system(build_path + " ../installer/installer_agent.sln /t:Rebuild /p:Configuration=Release;Platform=Win32")') != 0:
		sys.exit(1)

log_and_exec('sign_file(os.path.abspath("../bin/Release/installer/installer.exe"))')

if (is_icq):
	if log_and_eval('os.system(build_path + " ../installerpack/installerpack.sln /t:Rebuild /p:Configuration=Release;Platform=Win32")') != 0:
		sys.exit(1)

	log_and_exec('sign_file(os.path.abspath("../bin/Release/installer/icqsetup.exe"))')
else:
	if log_and_eval('os.system(build_path + " ../installerpack_agent/installerpack.sln /t:Rebuild /p:Configuration=Release;Platform=Win32")') != 0:
		sys.exit(1)

	log_and_exec('sign_file(os.path.abspath("../bin/Release/installer/magentsetup.exe"))')

upload_folder = os.path.join(os.environ["STORAGE"], build_number).replace('\\', '/')

if not os.path.exists(upload_folder):
	os.mkdir(upload_folder)
	
symbols = os.path.join(upload_folder, "symbols").replace('\\', '/')
	
upload_folder = os.path.join(upload_folder, "Release").replace('\\', '/')

if not os.path.exists(upload_folder):
	os.mkdir(upload_folder)

log_and_exec('distutils.dir_util.copy_tree("../bin/Release", upload_folder)');

if not os.path.exists(symbols):
	os.mkdir(symbols)

log_and_exec('shutil.copyfile(os.path.join(upload_folder, "corelib.exp"), os.path.join(symbols, "corelib.exp"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "corelib.lib"), os.path.join(symbols, "corelib.lib"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "corelib.pdb"), os.path.join(symbols, "corelib.pdb"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "coretest.pdb"), os.path.join(symbols, "coretest.pdb"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "icq.exp"), os.path.join(symbols, "icq.exp"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "icq.lib"), os.path.join(symbols, "icq.lib"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "icq.pdb"), os.path.join(symbols, "icq.pdb"))');
log_and_exec('shutil.copyfile(os.path.join(upload_folder, "libvoip_x86.lib"), os.path.join(symbols, "libvoip_x86.lib"))');

sys.exit(0)