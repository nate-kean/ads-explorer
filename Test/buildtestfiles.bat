::Pre:: pwd is .\Test\
@echo off

pushd Files
    mkdir 0streams

    echo 1stream.txt DATA > 1stream.txt

    echo 2streams.txt DATA > 2streams.txt
    echo 2streams.txt alternate1 > 2streams.txt:alternate1

    echo 3streams.txt DATA > 3streams.txt
    echo 3streams.txt alternate1 > 3streams.txt:alternate1
    echo 3streams.txt alternate2 > 3streams.txt:alternate2
popd
