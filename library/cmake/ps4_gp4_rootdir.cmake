# ps4 platform
# Rewrites the <rootdir> of a gp4 project from the paths of its <file> entries.
# create-gp4 drops a directory whose name starts another one beside it
# (i18n/es next to i18n/es-419), and PkgTool then fails on its files with
# "Sequence contains no elements".
# usage: cmake -DGP4=<project.gp4> -P ps4_gp4_rootdir.cmake

if (NOT GP4)
    message(FATAL_ERROR "GP4 is not defined")
endif ()

file(READ "${GP4}" content)

string(REGEX MATCHALL "targ_path=\"[^\"]*\"" paths "${content}")
set(dirs "")
foreach (path IN LISTS paths)
    string(REGEX REPLACE "^targ_path=\"(.*)\"$" "\\1" path "${path}")
    get_filename_component(dir "${path}" DIRECTORY)
    while (NOT dir STREQUAL "")
        list(APPEND dirs "${dir}")
        get_filename_component(dir "${dir}" DIRECTORY)
    endwhile ()
endforeach ()
list(REMOVE_DUPLICATES dirs)
list(SORT dirs)

function(emit_dirs parent indent out)
    set(xml "")
    foreach (dir IN LISTS dirs)
        get_filename_component(dir_parent "${dir}" DIRECTORY)
        if (dir_parent STREQUAL parent)
            get_filename_component(name "${dir}" NAME)
            emit_dirs("${dir}" "${indent}\t" children)
            if (children STREQUAL "")
                string(APPEND xml "${indent}<dir targ_name=\"${name}\"></dir>\n")
            else ()
                string(APPEND xml "${indent}<dir targ_name=\"${name}\">\n${children}${indent}</dir>\n")
            endif ()
        endif ()
    endforeach ()
    set(${out} "${xml}" PARENT_SCOPE)
endfunction()

emit_dirs("" "\t\t" rootdir)

string(FIND "${content}" "<rootdir>" begin)
string(FIND "${content}" "</rootdir>" end)
if (begin EQUAL -1 OR end EQUAL -1)
    message(FATAL_ERROR "No <rootdir> in ${GP4}")
endif ()
string(SUBSTRING "${content}" 0 ${begin} head)
string(SUBSTRING "${content}" ${end} -1 tail)
file(WRITE "${GP4}" "${head}<rootdir>\n${rootdir}\t${tail}")
