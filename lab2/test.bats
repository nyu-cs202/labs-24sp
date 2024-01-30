#!/usr/bin/env bats
setup() {
   export TEST_DIR="d$(date '+%Y%m%d%H%M%S')"
   ./mktest.sh ${TEST_DIR}
}

teardown() {
    rm ${TEST_DIR}/a
    rm ${TEST_DIR}/b
    rm ${TEST_DIR}/c
    rm ${TEST_DIR}/d
    rm ${TEST_DIR}/.boo
    rm ${TEST_DIR}/in0/a
    rm ${TEST_DIR}/in0/b
    rm ${TEST_DIR}/in0/in1/in2/x
    rm ${TEST_DIR}/.hidden/a
    rm ${TEST_DIR}/.hidden/b
    rm ${TEST_DIR}/.hidden/c
    rm ${TEST_DIR}/s{1,2,3,4}
    sudo rm ${TEST_DIR}/u*
    sudo rm ${TEST_DIR}/bad/bad_user ${TEST_DIR}/bad/bad_group ${TEST_DIR}/bad/bad_ugroup
    rmdir ${TEST_DIR}/bad
    rmdir ${TEST_DIR}/in0/in1/in2
    rmdir ${TEST_DIR}/in0/in1
    rmdir ${TEST_DIR}/in0
    rmdir ${TEST_DIR}/.hidden
    rmdir ${TEST_DIR}
}

@test "ls does not throw errors" {
    run ./ls
    [ $status -eq 0 ]
}

@test "ls -l throws correct error on bad uid and group" {
    run ./ls -l ${TEST_DIR}/bad
    [ $status -eq 96 ]
}

@test "implementation output has same number of lines ls -1" {
    result=$(./ls | wc -l)
    compare=$(ls -1 | wc -l)
    [ "$result" -eq "$compare" ]
}

@test "implementation output has same files as ls -1" {
    result=$(./ls | awk '{print $1}' | sort)
    # --file-type here is to get rid of the '*' that follows executables, etc.
    compare=$(ls -1 --color=never --file-type | awk '{print $1}'| sed 's/@$//g' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -l" {
    result=$(./ls -l ${TEST_DIR} | awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort)
    compare=$(ls -1l --color=never --file-type ${TEST_DIR} | grep -v "^total"| sed 's/@$//g' | awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -a" {
    result=$(./ls -a ${TEST_DIR} | awk '{print $1}' | sort)
    compare=$(ls -1a --color=never --file-type ${TEST_DIR} | grep -v "^total"| sed 's/@$//g' | sed 's!\./!.!' | awk '{print $1}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -R" {
    result=$(./ls -R ${TEST_DIR} | sed -r '/^\s*$/d' |  awk '{print $1}' | sort) 
    compare=$(ls -1R --color=never --file-type ${TEST_DIR} | grep -v "^total"| sed 's/@$//g' | sed -r '/^\s*$/d'  | awk '{print $1}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -al" {
    result=$(./ls -al ${TEST_DIR} | awk '{print $1}' | sort)
    compare=$(ls -1al --color=never --file-type ${TEST_DIR} | grep -v "^total"| sed 's/@$//g' | sed 's!\./!.!' | awk '{print $1}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -lR" {
    result=$(./ls -lR ${TEST_DIR} | sed -r '/^\s*$/d' | awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort )
    compare=$(ls -1lR --color=never --file-type ${TEST_DIR} | grep -v "^total" | sed -r '/^\s*$/d'| sed 's/@$//g' |awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -alR" {
    result=$(./ls -alR ${TEST_DIR} | sed -r '/^\s*$/d'  | awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort)
    compare=$(ls -1alR --color=never --file-type ${TEST_DIR} | grep -v "^total" | sed -r '/^\s*$/d'| sed 's!\./!.!' | sed 's/@$//g' |awk '{print $1,$2,$3,$4,$5,$6,$7,$8,$9}' | sort)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -n" {
    result=$(./ls -n ${TEST_DIR})
    compare=$(ls -1 ${TEST_DIR} | wc -l)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "implementation -n flag takes precedence over l" {
    result=$(./ls -ln ${TEST_DIR})
    compare=$(ls -1l ${TEST_DIR} | grep -v "^total" | wc -l)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -an" {
    result=$(./ls -an ${TEST_DIR})
    compare=$(ls -1a ${TEST_DIR} | wc -l)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -nR" {
    result=$(./ls -nR ${TEST_DIR})
    compare=$(ls -1lR --color=never --file-type ${TEST_DIR} | grep -v "^total" | sed '/:$/d' | sed -r '/^\s*$/d'| sed 's/@$//g' | wc -l)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check ls -alnR" {
    result=$(./ls -alnR ${TEST_DIR})
    compare=$(ls -1alR --color=never --file-type ${TEST_DIR} | grep -v "^total" | sed '/:$/d' | sed -r '/^\s*$/d'| sed 's/@$//g' | wc -l)
    if [ "$result" != "$compare" ]; then
        printf "Failed: Diff between output and expected:\n"
        diff <(echo "$result") <(echo "$compare")
    fi
    [ "$result" = "$compare" ]
}

@test "check error handling in the absence of file" {
    run ./ls ${TEST_DIR}/x
    [ $status -eq 72 ] 
}
