which is correct?

  [ req ]
default_bits            = 2048
encrypt_key             = no
default_md              = sha256
utf8                    = yes
string_mask             = utf8only
prompt                  = no
distinguished_name = req_distinguished_name
req_extensions     = req_ext
[ req_distinguished_name ]
countryName         = IN
stateOrProvinceName = MAHARASHTRA
localityName        = MUMBAI
organizationName    = kotak Mahindra Bank Ltd
organizationalUnitName = Kotak811
commonName          = blportal.uat.kotak811.bank.in
[ req_ext ]
subjectAltName = @alt_names
[alt_names]
DNS.1 = blportal.uat.kotak811.bank.in
DNS.2 = blportal.uat.kotak811.com


  or



  [ req ]
default_bits            = 2048
encrypt_key             = no
default_md              = sha256
utf8                    = yes
string_mask             = utf8only
prompt                  = no
distinguished_name = req_distinguished_name
req_extensions     = req_ext
[ req_distinguished_name ]
countryName         = IN
stateOrProvinceName = MAHARASHTRA
localityName        = MUMBAI
organizationName    = kotak Mahindra Bank Ltd
organizationalUnitName = Kotak811
commonName          = blportal.uat.kotak811.bank.in
[ req_ext ]
subjectAltName = @alt_names
[alt_names]
DNS.1 = blportal.uat.kotak811.com
