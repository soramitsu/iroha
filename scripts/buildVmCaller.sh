#!/bin/sh

go get github.com/hyperledger/burrow/execution/evm \
  github.com/hyperledger/burrow/acm \
  github.com/hyperledger/burrow/binary \
  github.com/hyperledger/burrow/crypto \
  github.com/hyperledger/burrow/logging \
  github.com/tmthrgd/go-hex \
  golang.org/x/crypto/ripemd160

# build vmCall.so and vmCall.h
cd $GOPATH/src/vmCaller
go build -o vmCall.so -buildmode=c-shared main.go
cp $GOPATH/src/vmCaller/vmCall.so /opt/iroha/irohad/ametsuchi/
cp $GOPATH/src/vmCaller/vmCall.h /opt/iroha/irohad/ametsuchi/
