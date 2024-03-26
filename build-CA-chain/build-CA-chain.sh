#!/bin/sh

#
# BSD 2-Clause License
#
# Copyright (c) 2024, Sashan
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# script builds ROOT CA (sniffles) and two intermediate CAs (shifty, lifty)
# which certificates are signed by root CA sniffles.
# It should look like this
#	sniffles
#		|
#		+---- lifty
#		|
#		'---- shifty
#
# Then we ask lifty to create server certificate for server lifty (sorry for
# confusion). We also ask CA shifty to create a client certificate for petunia
# user.
#
# I followed chapter 1.5 of  OpenSSL CoookBook [1] to create all CAs and
# server certificate for host lifty.
#
# To create client certificate I used tips found on stack exchange. The links
# are found in comments.
#
# The script runs in batch mode, there is no interaction. It's intentional
# because the primary goal is to quickly generate certificates for testing.
# The client and server certificates are left in $CA_DIR/certs.out
# (./CA/certs.out).
#
# The CA certificates are found in those directories:
#	CA/sniffles/sniffles.cert
#	CA/lifty/lifty.cert
#	CA/shifty/shifty.cert
# private keys for CAs can be found here:
#	CA/sniffles/private/sniffles.key
#	CA/lifty/private/lifty.key
#	CA/shifty/private/shifty.key
#
# [1] https://www.feistyduck.com/library/openssl-cookbook/online/openssl-command-line/private-ca.html
#

export PATH=$OPENSSL_PATH/bin:$PATH
export LD_LIBRARY_PATH=$OPENSSL_PATH/lib
export MANPATH=$OPENSSL_PATH/share/man

OPENSSL=$OPENSSL_PATH/bin/openssl
CA_DIR=`pwd`/CA
CERTS_OUT=$CA_DIR/certs.out
CA_ROOT=sniffles	# ROOT-CA is sniffles


function generate_rootCA_openssl_cnf {
	CA_ROOT_DIR=$1
	NAME=$2
	DOMAIN=$3
	CNF_FILE=$4
cat >$CNF_FILE <<EOF
#
# Configuration comes from feisty duck. This is the root-CA for internal
# testing
#
# https://www.feistyduck.com/library/openssl-cookbook/online/openssl-command-line/private-ca-creating-root.html
#
# The first section contains basic info about root CA
#
[default]
name                    = $NAME
domain_suffix           = $DOMAIN
aia_url                 = http://\$name.\$domain_suffix/\$name.cert
crl_url                 = http://\$name.\$domain_suffix/\$name.crl
ocsp_url                = http://ocsp.\$name.\$domain_suffix:9080
default_ca              = ca_default
name_opt                = utf8,esc_ctrl,multiline,lname,align

[ca_dn]
countryName             = "CZ"
organizationName        = "happy-tree-friends"
commonName              = "Root CA for happy-tree-friends"

#
# second section configures CA, so CA knows where
# its data are kept.
#
[ca_default]
home                    = $CA_ROOT_DIR
database                = \$home/db/index
serial                  = \$home/db/serial
crlnumber               = \$home/db/crlnumber
certificate             = \$home/\$name.cert
private_key             = \$home/private/\$name.key
RANDFILE                = \$home/private/random
new_certs_dir           = \$home/certs
unique_subject          = no
copy_extensions         = none
default_days            = 3650
default_crl_days        = 365
default_md              = sha256
policy                  = policy_c_o_match

#
# the third section defines how we generate
# root certificate for our root CA, this is
# used only once.
#
# Note this is test CA, unlike real CA found on internet
# we want to match on organizationName too. All certificates
# which will be part of root-CA chain will be using the same
# organizationNmae
#
[policy_c_o_match]
countryName             = match
stateOrProvinceName     = optional
organizationName        = match
organizationalUnitName  = optional
commonName              = supplied
emailAddress            = optional

[req]
default_bits            = 4096
encrypt_key             = yes
default_md              = sha256
utf8                    = yes
string_mask             = utf8only
prompt                  = no
distinguished_name      = ca_dn
req_extensions          = ca_ext

[ca_ext]
basicConstraints        = critical,CA:true
keyUsage                = critical,keyCertSign,cRLSign
subjectKeyIdentifier    = hash

#
# The fourth part defines parameters for certificates
# issued by root-CA. All certificates issued by
# root-CA will be for sub-ordinate CAs (sub_ca), hence constraint
# is set to CA:true.
#
# Note we alse set pathlen:0, this limits the length of
# CA-chain to one. There will be no sub-sub-CAs. The
# sub-CAs we certify by root-CA can not generate certificates
# for sub-sub-CAs.
#
# Also note we limit certificate key usage to clientAuth and
# serverAuth only. Certificates issued by sub-CA can't be
# used for anything else.
#
# Furthermore permitted:DNS constraint allows to issue
# certificates for hosts in $DOMAIN domain.
#
# details on excluded IPs can be found here:
# https://cabforum.org/working-groups/server/baseline-requirements/
#
# nameConstraints in sub_ca_ext is _not_ marked as critical.
# to make it criticial change the line to:
#	 nameConstraints         = critical,@name_constraints
#
# ommiting critical here improves interoperability
# with other SSL/TLS implementations which don't
# recognize 'critical'. Ommitting critical means the
# constraint can not be enforced.
#
# !!! sashan !!!
# looks like nameConstraints is fairly important.
# with current settings I'm not able to generate
# leaf certificate for server which would pass validation.
# openssl verify command fails with error:
#   C=CZ, O=happy-tree-friends, CN=lifty, emailAddress=webmaster@lifty.flatnet.local
#   error 47 at 0 depth lookup: permitted subtree violation
# the sub/altSubj in csr signed by CA reads as follows 
#    subjectAltName = DNS:lifty,DNS:lifty.flatnet.local,DNS:localhost,IP:127.0.0.1,IP:192.168.2.164
# 
# disabling nameConstraints makes validation to pass.
#
[sub_ca_ext]
authorityInfoAccess     = @issuer_info
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:true,pathlen:0
crlDistributionPoints   = @crl_info
extendedKeyUsage        = clientAuth,serverAuth
keyUsage                = critical,keyCertSign,cRLSign
#nameConstraints         = @name_constraints
subjectKeyIdentifier    = hash

[crl_info]
URI.0                   = \$crl_url

[issuer_info]
caIssuers;URI.0         = \$aia_url
OCSP;URI.0              = \$ocsp_url

[name_constraints]
permitted;DNS.0=$DOMAIN
#permitted;DNS.1=example.org
excluded;IP.0=0.0.0.0/0.0.0.0
excluded;IP.1=0:0:0:0:0:0:0:0/0:0:0:0:0:0:0:0

#
# this is OCSP (https://en.wikipedia.org/wiki/Online_Certificate_Status_Protocol)
#
[ocsp_ext]
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:false
extendedKeyUsage        = OCSPSigning
noCheck                 = yes
keyUsage                = critical,digitalSignature
subjectKeyIdentifier    = hash
EOF
}

function generate_interimCA_openssl_cnf {
	CA_ROOT_DIR=$1
	NAME=$2
	DOMAIN=$3
	CNF_FILE=$4
cat >$CNF_FILE <<EOF
#
# Configuration comes from feisty duck. This is the sub-CA for internal
# testing
#
# https://www.feistyduck.com/library/openssl-cookbook/online/openssl-command-line/private-ca-create-subordinate.html
#
# The first section contains basic info about root CA
#
[default]
name                    = $NAME
domain_suffix           = $DOMAIN
aia_url                 = http://\$name.\$domain_suffix/\$name.cert
crl_url                 = http://\$name.\$domain_suffix/\$name.crl
ocsp_url                = http://ocsp.\$name.\$domain_suffix:9080
default_ca              = ca_default
name_opt                = utf8,esc_ctrl,multiline,lname,align

[ca_dn]
countryName             = "CZ"
organizationName        = "happy-tree-friends"
commonName              = "CA for $NAME"

#
# second section configures CA, so CA knows where
# its data are kept.
#
# we've changed default_days to year
# we've changed copy_extensions to copy
# we may copy alternative names from CSR
# to certificate. this kinf of feature might not
# be always desired. In case of testing we don't care.
#
[ca_default]
home                    = $CA_ROOT_DIR
database                = \$home/db/index
serial                  = \$home/db/serial
crlnumber               = \$home/db/crlnumber
certificate             = \$home/\$name.cert
private_key             = \$home/private/\$name.key
RANDFILE                = \$home/private/random
new_certs_dir           = \$home/certs
unique_subject          = no
copy_extensions         = copy # !!
default_days            = 365
default_crl_days        = 30
default_md              = sha256
policy                  = policy_c_o_match

#
# the third section defines how we generate
# root certificate for our root CA, this is
# used only once.
#
# Note this is test CA, unlike real CA found on internet
# we want to match on organizationName too. All certificates
# which will be part of root-CA chain will be using the same
# organizationNmae
#
[policy_c_o_match]
countryName             = match
stateOrProvinceName     = optional
organizationName        = match
organizationalUnitName  = optional
commonName              = supplied
emailAddress            = optional

[req]
default_bits            = 4096
encrypt_key             = yes
default_md              = sha256
utf8                    = yes
string_mask             = utf8only
prompt                  = no
distinguished_name      = ca_dn
req_extensions          = ca_ext

[ca_ext]
basicConstraints        = critical,CA:true
keyUsage                = critical,keyCertSign,cRLSign
subjectKeyIdentifier    = hash

#
# The fourth part defines parameters for certificates
# issued by root-CA. All certificates issued by
# root-CA will be for sub-ordinate CAs (sub_ca), hence constraint
# is set to CA:true.
#
# Note we alse set pathlen:0, this limits the length of
# CA-chain to one. There will be no sub-sub-CAs. The
# sub-CAs we certify by root-CA can not generate certificates
# for sub-sub-CAs.
#
# Also note we limit certificate key usage to clientAuth and
# serverAuth only. Certificates issued by sub-CA can't be
# used for anything else.
#
# Furthermore permitted:DNS constraint allows to issue
# certificates for hosts in $DOMAIN domain.
#
# details on excluded IPs can be found here:
# https://cabforum.org/working-groups/server/baseline-requirements/
#
# nameConstraints in sub_ca_ext is _not_ marked as critical.
# to make it criticial change the line to:
#	 nameConstraints         = critical,@name_constraints
#
# ommiting critical here improves interoperability
# with other SSL/TLS implementations which don't
# recognize 'critical'. Ommitting critical means the
# constraint can not be enforced.
#
[sub_ca_ext]
authorityInfoAccess     = @issuer_info
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:true,pathlen:0
crlDistributionPoints   = @crl_info
extendedKeyUsage        = clientAuth,serverAuth
keyUsage                = critical,keyCertSign,cRLSign
nameConstraints         = @name_constraints
subjectKeyIdentifier    = hash

[crl_info]
URI.0                   = \$crl_url

[issuer_info]
caIssuers;URI.0         = \$aia_url
OCSP;URI.0              = \$ocsp_url

[name_constraints]
permitted;DNS.0=$DOMAIN
#permitted;DNS.1=example.org
excluded;IP.0=0.0.0.0/0.0.0.0
excluded;IP.1=0:0:0:0:0:0:0:0/0:0:0:0:0:0:0:0

#
# this is OCSP (https://en.wikipedia.org/wiki/Online_Certificate_Status_Protocol)
#
[ocsp_ext]
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:false
extendedKeyUsage        = OCSPSigning
noCheck                 = yes
keyUsage                = critical,digitalSignature
subjectKeyIdentifier    = hash

#
# these sections are required by sub-CA
#
[server_ext]
authorityInfoAccess     = @issuer_info
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:false
crlDistributionPoints   = @crl_info
extendedKeyUsage        = clientAuth,serverAuth
keyUsage                = critical,digitalSignature,keyEncipherment
subjectKeyIdentifier    = hash

[client_ext]
authorityInfoAccess     = @issuer_info
authorityKeyIdentifier  = keyid:always
basicConstraints        = critical,CA:false
crlDistributionPoints   = @crl_info
extendedKeyUsage        = clientAuth
keyUsage                = critical,digitalSignature
subjectKeyIdentifier    = hash
EOF
}

mkdir -p $CA_DIR
mkdir -p $CERTS_OUT

for i in $CA_ROOT shifty lifty ; do
	#
	# certs directory holds certificates issued by CA
	# db holds db of issued certificates and crls
	# private holds private keys for CA and OCSP
	#
	for j in certs db private  ; do
		mkdir -p $CA_DIR/$i/$j;
	done
	touch $CA_DIR/$i/db/index
	$OPENSSL rand -hex 16 > $CA_DIR/$i/db/serial
	echo 1111 > $CA_DIR/$i/db/crlnumber
done

#
# geneerate openssl.cnf for all CAs
#
generate_rootCA_openssl_cnf \
	$CA_DIR/$CA_ROOT sniffles local \
	$CA_DIR/$CA_ROOT/openssl.conf

#
# create root-CA sniffles.
#
# Note: there is no passphrase protection on keys we
# use for CAs and certificate itself. Everything here
# is for testing. In real life the CAs use passphrase
# as a minimal protection level. CA Keys are stored on
# HW tokens more likely. For server certificate keys there
# are some pros/cons to use passphrase see details here:
# https://www.feistyduck.com/library/openssl-cookbook/online/openssl-command-line/key-generation.html
# 
# for client/user certificates it depends. I feel passphrases
# should be more likely there.
#
# when opting for passphrase protection consider using
# option -aes-256-cbc (-aes-128-cbc) instead of -nodes
#
$OPENSSL req -new -config $CA_DIR/$CA_ROOT/openssl.conf \
	-out $CA_DIR/$CA_ROOT/$CA_ROOT.csr \
	-keyout $CA_DIR/$CA_ROOT/private/$CA_ROOT.key \
	-nodes # no passphrase
#
# create self-signed root-CA. We will use settings
# defined by ca_ext section in root-ca.conf we created
# by generate_rootCA_openssl_cnf.
#
# Note: we also use -batch, so there is no interaction
#
$OPENSSL ca -selfsign -config $CA_DIR/$CA_ROOT/openssl.conf \
	-in $CA_DIR/$CA_ROOT/$CA_ROOT.csr \
	-out $CA_DIR/$CA_ROOT/$CA_ROOT.cert \
	-batch -extensions ca_ext

#
# now we generate CRL in root-CA
#
$OPENSSL ca -gencrl -config $CA_DIR/$CA_ROOT/openssl.conf \
	-out $CA_DIR/$CA_ROOT/$i.crl

#
# create OCSP
#
$OPENSSL req -new -newkey rsa:2048 \
	-subj "/C=CZ/O=happy-tree-friends/CN=OCSP Root responder" \
	-keyout $CA_DIR/$CA_ROOT/private/$CA_ROOT-ocsp.key \
	-out $CA_DIR/$CA_ROOT/$CA_ROOT-ocsp.csr \
	-nodes
$OPENSSL ca -config $CA_DIR/$CA_ROOT/openssl.conf \
	-in $CA_DIR/$CA_ROOT/$CA_ROOT-ocsp.csr \
	-out $CA_DIR/$CA_ROOT/$CA_ROOT-ocsp.cert \
	-extensions ocsp_ext -days 30 -batch

#
# OCSP can be launched using $OPENSSL command:
#	$OPENSSL ocsp -port 9080 -index $CA_DIR/$CA_ROOT/db/index \
#		-rsigner $CA_DIR/$CA_ROOT/$CA_ROOT-oscp.cert \
#		-rkey $CA_DIR/$CA_ROOT/private/$CA_ROOT-ocsp.key \
#		-CA $CA_DIR/$CA_ROOT/$CA_ROOT.cert -text
#
#
# to test OCSP run command
#	$ openssl ocsp -issuer $CA_DIR/$CA_ROOT/root-ca.cert \
#		-CAfile $CA_DIR/$CA_ROOT/$CA_ROOT.cert \
#		-cert $CA_DIR/$CA_ROOT/$CA_ROOT-ocsp.cert \
#		-url http://127.0.0.1:9080
#

for i in shifty lifty ; do
	echo "Dump $i"
	generate_interimCA_openssl_cnf $CA_DIR/$i $i local \
		$CA_DIR/$i/openssl.conf
done

#
# create intermediate CAs shifty and lifty 
# we use root CA to sign lifty's and shifty's .csr
#
for i in shifty lifty ; do
	echo "creating $i"
	$OPENSSL req -new -config $CA_DIR/$i/openssl.conf \
		-out $CA_DIR/$i/$i.csr \
		-keyout $CA_DIR/$i/private/$i.key \
		-nodes
	$OPENSSL ca -config $CA_DIR/$CA_ROOT/openssl.conf \
		-in $CA_DIR/$i/$i.csr \
		-out $CA_DIR/$i/$i.cert \
		-extensions sub_ca_ext -batch
done

#
# here we generate server and client certificates.
# CA lifty generates server certificate for host
# lumpy.
#
# CA shifty generates client certificate for petunia.
#
# the certificate is shown before CA signs it.
# in real life you should pay attention to fields:
#	distinguished name
# 	basicConstraints
#	subjectAlternativeName
#
# server (host) certificates we typically need to
# deal with situation where the same service is available
# under two different name:
#	www.example.com
#	example.com
# in traditional sense we need a certificate for each
# name. Field Subject Alternative Nmae (SAN) solves
# that problem. You can create a single certificate
# with field as follows:
#	subjectAltName = DNS:*.example.com, DNS:example.com
# Keep in mind that using subjectAltName makes common names
# ignored (https://www.feistyduck.com/library/openssl-cookbook/online/openssl-command-line/creating-certificates-valid-for-multiple-hostnames.html)
#
#
# we are going to generate CSR for server lifty 
# we are going to use SAN. it will have names lifty, localhost 
#
# we create /tmp/lifty.conf file which configures openssl to generate CSR for
# lifty server. don't confuse lifty server with lifty-CA it's just unfortunate
# coincidence (result of my poor naming skills).
#
cat > /tmp/lifty.conf <<EOF
[req]
prompt = no
distinguished_name = dn
req_extensions = ext
input_password = PASSPHRASE

[dn]
CN = lifty
emailAddress = webmaster@lifty.flatnet.local
O = happy-tree-friends
L = Prague
C = CZ

#
# do not put IP addresses. they are not permitted see statement:
#     excluded;IP.1=0:0:0:0:0:0:0:0/0:0:0:0:0:0:0:0
# in CA configuration.
#
[ext]
#subjectAltName = DNS:lifty,DNS:lifty.flatnet.local,DNS:localhost,IP:127.0.0.1,IP:192.168.2.164
subjectAltName = DNS:ubuntu,DNS:localhost,IP:127.0.0.1,IP:172.16.1.10
EOF
$OPENSSL req -new -config /tmp/lifty.conf \
	-out $CA_DIR/certs.out/lifty.server.csr \
	-keyout $CA_DIR/certs.out/lifty.server.key \
	-nodes
rm -f /tmp/lifty.conf
#
# now we are going to ask lifty-CA to sign csr to
# obtain a certificate for lifty server.
# Note command below uses configuration for lifty-CA
#
$OPENSSL ca -config $CA_DIR/lifty/openssl.conf \
	-in $CA_DIR/certs.out/lifty.server.csr \
	-out $CA_DIR/certs.out/lifty.server.cert \
	-extensions server_ext -batch

#
# and now client/user certificate for user petunia.
# we will use shifty CA to create client certificate
# for petunia.
#
# feisty duck does not talk much about client certificates.
# client certificates seem to be deployed on case-by-case
# basis. It always depends on site which deploys them.
# answer from 2018 still seems to be up-to-date (2024)
# https://security.stackexchange.com/questions/192979/client-certificate-common-name-subject-alternative-name
#
# also section Subject Alternative Name in X509V3_CONFIG(5ossl) gives
# idea how to populate the field.
#
cat > /tmp/petunia.conf <<EOF
[req]
prompt = no
distinguished_name = dn
req_extensions = ext
input_password = PASSPHRASE

[dn]
CN = petunia
emailAddress = petunia@flatnet.local
O = happy-tree-friends
L = Prague
C = CZ

[ext]
subjectAltName = email:copy,dirName:dn
EOF
$OPENSSL req -new -config /tmp/petunia.conf \
	-out $CA_DIR/certs.out/petunia.client.csr \
	-keyout $CA_DIR/certs.out/petunia.client.key \
	-nodes
rm -f /tmp/petunia.conf
$OPENSSL ca -config $CA_DIR/shifty/openssl.conf \
	-in $CA_DIR/certs.out/petunia.client.csr \
	-out $CA_DIR/certs.out/petunia.client.cert \
	-extensions client_ext -batch

#
# certificate validation. there are many blogs and answers found
# by google. the reasonably accurate I could find is the one
# here:
#   https://security.stackexchange.com/questions/56389/ssl-certificate-framework-101-how-does-the-browser-actually-verify-the-validity
# 
# also details on how verification works for intermediate CAs (lifty and shifty) work
# there is an answer on stack exchange here:
#   https://superuser.com/questions/126121/how-to-create-my-own-certificate-chain
#

#
# also the verification process 
#
# verify certificate for lifty server. we use CA_ROOT (sniffles)  as a root
# certificate we ultimately trust.  Untrusted here is lifty CA (intermediate
# CA) which issued certificate for lifty server
#
$OPENSSL verify -x509_strict -CAfile $CA_DIR/$CA_ROOT/$CA_ROOT.cert \
	-untrusted $CA_DIR/lifty/lifty.cert \
	$CA_DIR/certs.out/lifty.server.cert

#
# verify certificate for petunia client. we use CA_ROOT (sniffles)
# as a root certificate we ultimately trust. Untrusted here is shifty CA
# (intermediate CA) which issued certificate for petunia client.
#
$OPENSSL verify -x509_strict -CAfile $CA_DIR/$CA_ROOT/$CA_ROOT.cert \
	-untrusted $CA_DIR/shifty/shifty.cert \
	$CA_DIR/certs.out/petunia.client.cert


