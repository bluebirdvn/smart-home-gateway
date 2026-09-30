FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
            file://fragment.cfg \
"

do_configure:prepend() {
    CERTFILE="${S}/certs/extract-cert.c"
    if [ -f "${CERTFILE}" ] && grep -q 'ENGINE_load_builtin_engines' "${CERTFILE}"; then
        sed -i '/else if (!strncmp(cert_src, "pkcs11:", 7)) {/,/write_cert(parms.cert);/c\
        } else if (!strncmp(cert_src, "pkcs11:", 7)) {\
                errx(1, "PKCS#11 support has been disabled in this build");' "${CERTFILE}"
        sed -i '/#include <openssl\/engine.h>/d' "${CERTFILE}"
    fi
}