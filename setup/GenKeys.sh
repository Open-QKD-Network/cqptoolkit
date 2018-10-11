#!/bin/bash
# Copyright (C) University of Bristol 2018
#    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#    See LICENSE file for details.
# Author Richard Collins <richard.collins@bristol.ac.uk>
# 

HOSTFQ=`hostname -f`
HOST=`hostname`
DOMAIN=`hostname -d`
IP4ADDR=`hostname -i | cut -d' ' -f1`
DAYS=365
#CURVE=secp384r1
CURVE=prime256v1
SERVER_FILE=${HOSTFQ}
ROOT_FILE=rootCA

SUBJ_C=GB
SUBJ_ST=City\ of\ Bristol
SUBJ_L=BRISTOL
SUBJ_O=University\ of\ Bristol
SUBJ_OU=QComms
SUBJ_CN=${HOSTFQ}
SUBJ_EA=${USER}@${HOSTFQ}

cat << EOF > config.txt
[req]
default_md = sha256
prompt = no
distinguished_name = req_dn
[ req_dn ]
commonName = ${HOSTFQ}
countryName = ${SUBJ_C}
stateOrProvinceName = ${SUBJ_ST}
localityName = ${SUBJ_L}
organizationName = ${SUBJ_O}
organizationalUnitName = ${SUBJ_OU}
emailAddress = ${SUBJ_EA}
EOF

cat << EOF > ext.txt
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature,keyEncipherment,dataEncipherment,nonRepudiation,keyAgreement,keyCertSign
extendedKeyUsage=serverAuth,clientAuth
subjectAltName = @alt_names
[alt_names]
DNS.0 = ${HOSTFQ}
DNS.2 = ${HOST}.local
DNS.3 = ${HOST}
DNS.4 = localhost
IP.1 = 127.0.0.1
IP.2 = ${IP4ADDR}
EOF


#echo Subject line: ${SUBJ}
if [ ! -f ${ROOT_FILE}.crt ]; then
        echo Generating root CA
        openssl req -x509 -new -nodes -sha256 \
            -newkey ec:<(openssl ecparam -name ${CURVE}) \
            -keyout ${ROOT_FILE}.key -out ${ROOT_FILE}.crt \
            -days ${DAYS} -config config.txt || exit 2
fi

#-newkey ec:<(openssl ecparam -name ${CURVE}) \
#-newkey rsa:2048 \

echo Generating ${SERVER_FILE}.csr
openssl req -new -sha256 -nodes -days ${DAYS} \
    -newkey ec:<(openssl ecparam -name ${CURVE}) \
    -keyout ${SERVER_FILE}.key -pubkey \
    -out ${SERVER_FILE}.csr \
    -config config.txt || exit 4

echo Generating ${SERVER_FILE}.crt

openssl x509 -req \
        -in ${SERVER_FILE}.csr \
        -CA ${ROOT_FILE}.crt -CAkey ${ROOT_FILE}.key \
        -out ${SERVER_FILE}.crt \
        -days ${DAYS} -sha256 -CAcreateserial -extfile ext.txt || exit 6

cat ${SERVER_FILE}.crt ${ROOT_FILE}.crt > ${SERVER_FILE}.pem

rm config.txt ext.txt ${SERVER_FILE}.csr
echo Done
echo "Usage: "
echo "   ${SERVER_FILE}.pem : Bundled certificates, use this as the 'cert' option for servers and the 'rootca' option for clients"
echo "   ${SERVER_FILE}.key : Private key, use this with the 'key' option for servers"
echo "   ${SERVER_FILE}.crt : Server certificate."
echo "   ${ROOT_FILE}.crt : Root certificate, can be used for trusted authority"
echo "   ${ROOT_FILE}.key : Root private key, keep this to generate multiple keys from the same authority"
echo "   ${ROOT_FILE}.srl : Serial file, keep this to ensure unique certificate serial numbers"