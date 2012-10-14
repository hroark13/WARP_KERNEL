#!/bin/bash
###the script use to delete comment

if [ -z "$1" ];then                 
     echo "ipput the source file"
     exit 1
fi

postfix=`echo $1 |cut -f 2 -d '.'` 

if [ -n "$2" ];then                   
     target_file=$2                  
     touch ${target_file}
else
prefix=`echo $1|cut -f 1 -d '.' ` 
     target_file=${prefix}.temp
     touch ${target_file}
fi
echo ${postfix}

case "postfix" in
          *) 
	      sed '/ZTE_MODIFY*/d' $1 >${target_file}
              ;;
        *)
              echo "c  or java program"
              sed 's/\/\*.*\*\///g' $1|sed '/\/\*/,/.*\*\//d' |\
              sed 's/\/\/.*//g' |\
              sed '/^[[:space:]]*$/d' |sed '/^$/d' >${target_file}
              echo "the source file is $1,target file is ${target_file}"
              ;;
        *)
              echo "unknown file type !"
              rm -f ${target_file}
              ;;
esac


