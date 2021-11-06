#!/bin/bash

##############################################################################
##
##  ScanGear MP for Linux
##  Copyright CANON INC. 2007-2010
##  All Rights Reserved.
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; version 2 of the License.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
##
##############################################################################

C_version="1.60-1"

L_INST_COM_01_01="Command executed = %s\n"

L_INST_COM_01_02="An error occurred. The package management system cannot be identified.\n"
L_INST_COM_01_03="An error occurred. A necessary package could not be found in the proper location.\n"
L_INST_COM_01_04="Installation has been completed.\n"
L_INST_COM_01_05="An error occurred. Your environment cannot be identified as 32-bit or 64-bit.\nTry to install again by using the following command.\n"

L_INST_COM_01_06="An error occurred. The specified environment differs from your environment.\nTry to install again by using the following command.\n"

L_INST_COM_02_01="Usage: %s\n"

L_INST_COM_02_02="Uninstallation has been completed.\n"

L_INST_COM_03_01="[Package]\n"



C_main_module="scangearmp"

#################################################################
#### C_function name define
#################################################################
C_function01="S_FUNC_show_inst_complete"
C_function02="S_FUNC_no_action"
C_function03="S_FUNC_show_copyright"

#################################################################
#### Show "install complete" message  $1:L_INST_COM_01_04
#### C_function01
#################################################################
S_FUNC_show_inst_complete(){
	printf "$1"
}

#################################################################
#### No action, case scanner
#### C_function02
#################################################################
S_FUNC_no_action(){
	echo -n ""
}

#################################################################
#### Show the copyright  $1:version
#### C_function03
#################################################################
S_FUNC_show_copyright()
{
	s_copyright1="ScanGear MP Ver.$1 for Linux"
	s_copyright2="Copyright CANON INC. 2007-2010"
	s_copyright3="All Rights Reserved."

	echo $s_copyright1; echo $s_copyright2; echo $s_copyright3
}

C_ERR_CODE="128"
C_big="50"
C_equal="40"
C_small="30"

C_common="common"
C_system=""
C_arch=""
C_arch32="32"
C_arch64="64"

C_err="no_error"
C_err_unknown="err_unknown"
C_err_mismatch="err_mismatch"
C_err_usage="err_usage"

C_install_script_fname="install.sh"
C_config_path_rpm="/usr/local/bin"
C_config_path_deb="/usr/bin"

C_copyrightb="=================================================="

C_arg_inst="[ --bit32 | --bit64 ]"
C_arg_pkg="[ --uninstall | --version ]"

C_FUNC_show_and_exec()
{
	printf "$L_INST_COM_01_01" "$1"
	$1
}

C_FUNC_version_comp()
{
	local c_tmpstr=""
	local c_ver1=""
	local c_ver2=""
	local c_rela1=""
	local c_reln1=""
	local c_rela2=""
	local c_reln2=""
	
	C_FUNC_makelist()
	{
		echo $1
		echo $2
	}

	## version compare ##
	# ex. 3.10->310, 1.30->130 #
	c_tmpstr=`echo ${1%%-*}`
	c_ver1=`echo ${c_tmpstr%%.*}``echo ${c_tmpstr##*.}`
	c_tmpstr=`echo ${2%%-*}`
	c_ver2=`echo ${c_tmpstr%%.*}``echo ${c_tmpstr##*.}`

	# ex. 310 > 300  #
	if [ $c_ver1 -gt $c_ver2 ]; then
		return $C_big
	elif [ $c_ver1 -lt $c_ver2 ]; then
		return $C_small
	fi
	
	## release compare ##
	# ex. a13->[a][13], 2->[][2] #
	c_tmpstr=`echo ${1##*-}`
	c_rela1=`echo ${c_tmpstr%%[0-9]*}`
	c_reln1=`echo ${c_tmpstr##*[a-z]}`
	c_tmpstr=`echo ${2##*-}`
	c_rela2=`echo ${c_tmpstr%%[0-9]*}`
	c_reln2=`echo ${c_tmpstr##*[a-z]}`

	# ex. [a][13] < [][2] #
	if [ -z $c_rela1 ] && [ -n $c_rela2 ]; then
		return $C_big
	elif [ -n $c_rela1 ] && [ -z $c_rela2 ]; then
		return $C_small
	fi
	
	# ex. [a][2] < [b][1] #
	if [ -n $c_rela1 ] && [ $c_rela1 != $c_rela2 ]; then
		list=`C_FUNC_makelist $c_rela1 $c_rela2 | sort`
		for c_tmpstr in $list; do
			if [ $c_tmpstr = $c_rela1 ]; then
				return $C_small
			else
				return $C_big
			fi
		done
	fi
	
	# ex. [a][2] > [a][1], [b][9] < [b][10] #
	if [ $c_reln1 -gt $c_reln2 ]; then
		return $C_big
	elif [ $c_reln1 -lt $c_reln2 ];then
		return $C_small
	else
		return $C_equal
	fi
}

C_FUNC_get_system()
{
	local c_system_rpm=""
	local c_system_deb=""

	## Judge is the distribution supporting rpm? ##
	rpm --version 1> /dev/null 2>&1
	c_system_rpm=$?

	## Judge is the distribution supporting dpkg(debian)? ##
	dpkg --version 1> /dev/null 2>&1
	c_system_deb=$?

	## rpm and deb are error, or rpm and deb are no error, is error ##
	if [ $c_system_rpm = 0 -a $c_system_deb = 0 ] || [ $c_system_rpm != 0 -a $c_system_deb != 0 ]; then
		printf "$L_INST_COM_01_02"
		return $C_ERR_CODE
	else
		if test $c_system_rpm -eq 0; then
			C_system="rpm"
			C_arch32="i386"
			C_arch64="x86_64"
		else
			C_system="deb"
			C_arch32="i386"
			C_arch64="amd64"
		fi
	fi
	
	return 0
}

C_FUNC_get_bitconf()
{
	local c_bit_conf=""
	local c_sudo_command=""
	local c_arg1=$1

	if [ $C_system = "deb" ]; then
		c_sudo_command="sudo "
	fi

	getconf LONG_BIT 1> /dev/null 2>&1
	if [ $? -eq 0 ]; then
		c_bit_conf=`getconf LONG_BIT`
	else
		c_bit_conf=""
	fi

	if [ -z $C_arch ]; then
		# No argment and getconf=32|64 -> continue #
		if [ -z $c_bit_conf ]; then
			if [ -z $c_arg1 ]; then
				C_err=$C_err_unknown
			elif [ $c_arg1 = "version" ]; then
				C_arch="*"
			fi
		elif [ $c_bit_conf = "32" ]; then
			C_arch=$C_arch32
		elif [ $c_bit_conf = "64" ]; then
			C_arch=$C_arch64
		else
			C_err=$C_err_unknown
		fi
	else
		if [ $C_arch = "32" ]; then
			# --bit32 and getconf=error -> continue #
			if [ -z $c_bit_conf ]; then
				C_arch=$C_arch32
			elif [ $c_bit_conf = "32" ]; then
				C_arch=$C_arch32
			else
				C_err=$C_err_mismatch
			fi
		elif [ $C_arch = "64" ]; then
			# --bit64 and getconf=error -> continue #
			if [ -z $c_bit_conf ]; then
				C_arch=$C_arch64
			elif [ $c_bit_conf = "64" ]; then
				C_arch=$C_arch64
			else
				C_err=$C_err_mismatch
			fi
		fi
	fi

	if [ $C_err = $C_err_mismatch ]; then
		printf "$L_INST_COM_01_06"
		printf "\n  ${c_sudo_command}${0}\n\n"
		return $C_ERR_CODE
	elif [ $C_err = $C_err_unknown ]; then
		printf "$L_INST_COM_01_05"
		printf "\n  ${c_sudo_command}${0} ${C_arg_inst}\n\n"
		return $C_ERR_CODE
	fi	
	
	return 0
}

C_FUNC_localize()
{
	local lc_file_dir=$1

	## Get current LANG information ##
	local current_lang=`echo $LANG | tr '[:upper:]' '[:lower:]'`

	## Get printer or scanner ##
	local driver=""
	if [ $C_main_module = "cnijfilter" ]; then
		driver="printer"
	else
		driver="scanner"
	fi

	## Get localize file name ##
	local lc_file="nolocalize"
	case "${current_lang##*.}" in
		utf8 | utf-8)
			case "${current_lang%%.*}" in
				ja_jp)
					lc_file="${driver}_ja_utf8.lc"
					;;
				fr_fr)
					lc_file="${driver}_fr_utf8.lc"
					;;
				zh_cn)
					lc_file="${driver}_zh_utf8.lc"
					;;
			esac
			;;
	esac

	## Set localize file ##
	if [ $lc_file != "nolocalize" ]; then
		if [ -f ${lc_file_dir}/${lc_file} ]; then
			source ${lc_file_dir}/${lc_file}
		fi
	fi
	
	return 0
}

######################################################
#### _ _ E x e c u t e _ I n s t a l l . s h  _ _ ####
######################################################
if [ ${0##*/} = $C_install_script_fname ]; then

	C_argment=$1

    ################
	### Localize ###
    ################
    C_local_path_inst="`echo $(dirname $0)`/resources"
	C_FUNC_localize "$C_local_path_inst"

	#########################
	### Check the argment ###
	#########################
	if [ $# -eq 1 ]; then
		if  [ $C_argment = "--bit32" ]; then
			C_arch=$C_arch32
		elif [ $C_argment = "--bit64" ]; then
			C_arch=$C_arch64
		else
			C_err=$C_err_usage
		fi
	elif [ $# -ne 0 ]; then
		C_err=$C_err_usage
	fi

	if [ $C_err = $C_err_usage ]; then
		printf "$L_INST_COM_02_01" "${0##*/} $C_arg_inst"
		exit
	fi

	#################################################################
	#### _ _ B o t h _ P a c k a g e _ C o m m o n _ F l o w _ _ ####
	#################################################################

	## Show the copyright ##
	if test `id -un` = "root"; then
		echo $C_copyrightb; echo
		$C_function03 "$C_version"
		echo ; echo $C_copyrightb
	fi

    #################################
	### Judge distribution system ###
    #################################

	C_FUNC_get_system
	if [ $? -ne 0 ]; then
		exit
	fi

    #########################
	### Judge 32bit/64bit ###
    #########################

	C_FUNC_get_bitconf
	if [ $? -ne 0 ]; then
		exit
	fi

    ############################
	### Check file structure ###
    ############################

	## Get full path of script and packages ##
	C_pkg_path=`echo $(dirname $0)`/packages
	if [ ! -d $C_pkg_path ]; then
		printf "$L_INST_COM_01_03"
		exit
	fi

	## Count total files and check the filename ##
	C_file_cnt=0
	C_files=$C_pkg_path/*${C_arch}*
	for filename in $C_files; do
		if [ $filename != $C_pkg_path ]; then
			# Count number of C_files           #
			C_file_cnt=`expr $C_file_cnt + 1`
			
			# Get only file name              #
			C_onlyfname=`echo ${filename##*/}`
			# Check the extension, and the filename for module #
			if [ `echo ${C_onlyfname##*.}` != $C_system -o `echo ${C_onlyfname%%-*}` != $C_main_module ]; then
				printf "$L_INST_COM_01_03"
				exit
			fi

			# Get device name(ex.mx860series) #
			C_tmpstr=`echo ${C_onlyfname##$C_main_module-}`
			if [ $C_system = "rpm" ]; then
				if [ `echo ${C_tmpstr%%-$C_version*}` != $C_common ]; then
					C_device=`echo ${C_tmpstr%%-$C_version*}`
				fi
			else
				if [ `echo ${C_tmpstr%%_$C_version*}` != $C_common ]; then
					C_device=`echo ${C_tmpstr%%_$C_version*}`
				fi
			fi
		fi
	done

	## Check number of C_files ##
	if [ $C_file_cnt -ne 2 ]; then
		printf "$L_INST_COM_01_03"
		exit
	fi

	## Recheck package names ##
	C_pkgname_common=$C_main_module-$C_common
	C_pkgname_depend=$C_main_module-$C_device
	if [ $C_system = "rpm" ]; then
		C_fpath_common=$C_pkg_path/$C_pkgname_common-$C_version.$C_arch.$C_system
		C_fpath_depend=$C_pkg_path/$C_pkgname_depend-$C_version.$C_arch.$C_system
	else
		C_fpath_common=$C_pkg_path/${C_pkgname_common}_${C_version}_$C_arch.$C_system
		C_fpath_depend=$C_pkg_path/${C_pkgname_depend}_${C_version}_$C_arch.$C_system
	fi

	## Check having common and depend package files ##
	if [ ! -f $C_fpath_common ] || [ ! -f $C_fpath_depend ]; then
		printf "$L_INST_COM_01_03"
		exit
	fi

	#####################################################################
	#### _ _ P a c k a g e _ S y s t e m _ D e p e n d _ F l o w _ _ ####
	#####################################################################

	C_FUNC_rpm_install_process()
	{
		local c_fpath_pkg_name=$1
		local c_pkg_name=$2
		local c_exec_update=1
		local c_installed_pkg=""
		
		## result -> 0:Package installed, 1:Package not installed ##
		c_installed_pkg=`rpm -q $c_pkg_name`
		if [ $? -eq 0 ]; then
			c_installed_pkg_ver=`echo ${c_installed_pkg##$c_pkg_name-}`
			c_installed_pkg_ver=`echo ${c_installed_pkg_ver%%.$C_arch32}`
			c_installed_pkg_ver=`echo ${c_installed_pkg_ver%%.$C_arch64}`
			C_FUNC_version_comp $c_installed_pkg_ver $C_version
			if [ $? -ne $C_small ]; then
				c_exec_update=0
			fi
		fi

		if [ $c_exec_update -eq 1 ]; then
			C_FUNC_show_and_exec "rpm -Uvh $c_fpath_pkg_name"
			if [ $? -ne 0 ]; then
				return $C_ERR_CODE
			fi
		else
			C_FUNC_show_and_exec "rpm --test -U $c_fpath_pkg_name"
		fi

		return 0	
	}

	C_FUNC_deb_install_process()
	{
		local c_fpath_pkg_name=$1

		## result -> 0:Install process complete, 1:Install process depend error ##
		C_FUNC_show_and_exec "sudo dpkg -iG $c_fpath_pkg_name"
		if [ $? != 0 ]; then
			return $C_ERR_CODE
		fi
		
		return 0
	}

	## Make Package-config script, after Check Newer version config script is installed already ##
	C_FUNC_make_pkgconfig()
	{
		local c_pkgconfig_fname=$1
		local c_script_path=$2
		local c_sudo_command=$3
		local c_pkgconfig_fpath="$c_script_path/$c_pkgconfig_fname"
		local c_config_path=""

		if [ ! -d /usr ]; then
			$c_sudo_command mkdir /usr
		fi
		if [ $2 = "rpm" ]; then
			if [ ! -d /usr/local ]; then
				mkdir /usr/local
			fi
			c_config_path=$C_config_path_rpm
		else
			c_config_path=$C_config_path_deb
		fi
		if [ ! -d $c_config_path ]; then
			$c_sudo_command mkdir $c_config_path
		fi

		$c_sudo_command cp -af $(dirname $0)/$C_install_script_fname $c_pkgconfig_fpath

		## Change file permission to same install.sh ##
		$c_sudo_command chmod 755 $c_pkgconfig_fpath
		$c_sudo_command chown root $c_pkgconfig_fpath
		$c_sudo_command chgrp root $c_pkgconfig_fpath
	}

	## Copy Localize-file, after making pkeconfig script ##
	C_FUNC_copy_lcfile()
	{
		local c_lcfile_srcpath=$1
		local c_lcfile_dstname=$2
		local c_sudo_command=$3
		local c_lcfile_dstpath=""

		if [ ! -d /usr ]; then
			$c_sudo_command mkdir /usr
		fi
		if [ $C_system = "rpm" ]; then
			if [ ! -d /usr/local ]; then
				mkdir /usr/local
			fi
			if [ ! -d /usr/local/share ]; then
				mkdir /usr/local/share
			fi
			c_lcfile_dstpath="/usr/local/share"
		else
			if [ ! -d /usr/share ]; then
				$c_sudo_command mkdir /usr/share
			fi
			c_lcfile_dstpath="/usr/share"
		fi

		$c_sudo_command rm -rf $c_lcfile_dstpath/$c_lcfile_dstname
		$c_sudo_command mkdir $c_lcfile_dstpath/$c_lcfile_dstname

		$c_sudo_command cp -a $c_lcfile_srcpath/*.lc $c_lcfile_dstpath/$c_lcfile_dstname/
	}

	if [ $C_system = "rpm" ]; then
		C_install_process="C_FUNC_rpm_install_process"
		C_uninstall_command="rpm -e"
		C_script_path=$C_config_path_rpm
		C_sudo_command=""
		
		##  Check permission by root ##
		if test `id -un` != "root"; then
			su -c "$0 $C_argment"
			exit
		fi
	else
		C_install_process="C_FUNC_deb_install_process"
		C_uninstall_command="sudo dpkg -P"
		C_script_path=$C_config_path_deb
		C_sudo_command="sudo"
	fi

	## Common-Package install process ##
	$C_install_process $C_fpath_common $C_main_module-$C_common
	if [ $? -ne 0 ]; then
		if [ $C_system = "deb" ]; then
			C_FUNC_show_and_exec "$C_uninstall_command $C_pkgname_common"
		fi
		exit
	fi
		
	## Depend-Package install process ##
	$C_install_process $C_fpath_depend $C_main_module-$C_device
	if [ $? -ne 0 ]; then
		if [ $C_system = "deb" ]; then
			C_FUNC_show_and_exec "$C_uninstall_command $C_pkgname_depend"
		fi
		C_FUNC_show_and_exec "$C_uninstall_command $C_pkgname_common"
		exit
	fi
		
	C_pkgconfig_fname=$C_main_module-$C_device-pkgconfig.sh
	C_pkgconfig_dname=${C_pkgconfig_fname%%\.sh}
	$C_pkgconfig_fname --pkgconfig 1> /dev/null 2>&1
	if [ $? -ne 0 ]; then
		C_FUNC_make_pkgconfig $C_pkgconfig_fname $C_script_path $C_sudo_command
		C_FUNC_copy_lcfile $C_local_path_inst $C_pkgconfig_dname $C_sudo_command
	else
		C_installed_config_ver=`$C_pkgconfig_fname --pkgconfig`
		C_FUNC_version_comp $C_installed_config_ver $C_version
		if [ $? -lt $C_equal ]; then
			$C_sudo_command rm -rf $C_script_path/$C_pkgconfig_fname
			C_FUNC_make_pkgconfig $C_pkgconfig_fname $C_script_path $C_sudo_command
			C_FUNC_copy_lcfile $C_local_path_inst $C_pkgconfig_dname $C_sudo_command
		fi
	fi
	
	$C_function01 "$L_INST_COM_01_04" "$C_device" "$C_system"

##########################################################
#### _ _ E x e c u t e _ P k g c o n f i g . s h  _ _ ####
##########################################################
else
	C_argment=$1

    #################################
	### Judge distribution system ###
    #################################
	C_FUNC_get_system
	if [ $? -ne 0 ]; then
		exit
	fi

	C_intermediate=${0##*$C_main_module-}
	C_device=${C_intermediate%%-*}

    ################
	### Localize ###
    ################
	if [ $C_system = "rpm" ]; then
		C_local_path_pkgconf="/usr/local/share/${C_main_module}-${C_device}-pkgconfig"
	else
		C_local_path_pkgconf="/usr/share/${C_main_module}-${C_device}-pkgconfig"
	fi
	C_FUNC_localize "$C_local_path_pkgconf"

	## Check the argment ##
	if [ $# -ne 1 ]; then
		C_err=$C_err_usage
	elif [ $C_argment != "--version" ] && [ $C_argment != "--uninstall" ] && [ $C_argment != "--pkgconfig" ]; then
		C_err=$C_err_usage
	fi

	if [ $C_err = $C_err_usage ]; then
		printf "$L_INST_COM_02_01" "${0##*/} $C_arg_pkg"
		exit
	fi

	########################
	### Unistall process ###
	########################
	if [ $C_argment = "--uninstall" ]; then
	
		C_FUNC_rpm_uninstall_process()
		{
			rpm -q $1 1> /dev/null 2>&1
			## result -> 0:Package installed, 1:Package not installed ##
			if [ $? -eq 0 ]; then
				# uninstall #
				C_FUNC_show_and_exec "rpm -e $1"
				## result -> 0:Uninstall complete, 1:Uninstall error by debendency ##
				if [ $? -ne 0 ]; then
					# Dependency error #
					return $C_ERR_CODE
				fi
			fi
			return 0
		}

		C_FUNC_deb_uninstall_process()
		{
			# uninstall #
			C_FUNC_show_and_exec "sudo dpkg -P $1"
			## result -> 0:Uninstall complete, 1:Uninstall error by debendency ##
			if [ $? -ne 0 ]; then
				# Dependency error #
				return $C_ERR_CODE
			fi
			
			return 0
		}

		if [ $C_system = "rpm" ]; then
			C_uninstall_process="C_FUNC_rpm_uninstall_process"
			C_sudo_command=""
			
			##  Check permission by root ##
			if test `id -un` != "root"; then
				su -c "$0 $C_argment"
				exit
			fi
		else
			C_uninstall_process="C_FUNC_deb_uninstall_process"
			C_sudo_command="sudo"
		fi

		$C_function02 "$C_device" "$C_system"

		##  Uninstall Depend-Package ##
		$C_uninstall_process $C_main_module-$C_device
		if [ $? -ne 0 ]; then
			exit
		fi

		##  Delete mine (pkgconfig.sh)  ##
		$C_sudo_command rm -rf $(dirname $0)/${0##*/}

		C_pkgconfigsh=${0##*/}
		C_pkgconfig=${C_pkgconfigsh%%\.sh}
		if [ $C_system = "rpm" ]; then
			C_lcfile_path="/usr/local/share/${C_pkgconfig}"
		else
			C_lcfile_path="/usr/share/${C_pkgconfig}"
		fi
		$C_sudo_command rm -rf $C_lcfile_path
		
		##  Uninstall Common-Package ##
		$C_uninstall_process $C_main_module-$C_common

		printf "$L_INST_COM_02_02"

	###############################
	### Version display process ###
	###############################
	elif [ $C_argment = "--version" ]; then

	    #########################
		### Judge 32bit/64bit ###
	    #########################
		 
		C_FUNC_get_bitconf version
		if [ $? -eq $C_ERR_CODE ]; then
			exit
		fi
				
		echo ; $C_function03 "$C_version"
		printf "\n$L_INST_COM_03_01"
		
		if [ $C_system = "rpm" ]; then
			echo $C_main_module-$C_common-$C_version.$C_arch.$C_system
			echo $C_main_module-$C_device-$C_version.$C_arch.$C_system
		else
			echo $C_main_module-${C_common}_${C_version}_$C_arch.$C_system
			echo $C_main_module-${C_device}_${C_version}_$C_arch.$C_system
		fi
		echo

	elif [ $C_argment = "--pkgconfig" ]; then
		echo $C_version
	fi

fi
