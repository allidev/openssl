/*
 * Copyright 2002-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include "../apps/apps.h"

#include "testutil.h"
#include "test_main_custom.h"

/* apps/apps.c depend on these */
char *default_config_file = NULL;
BIO *bio_err = NULL;

/* Old style PEM password callback */
static int test_pem_password_cb(char *buf, int size, int rwflag, void *userdata)
{
    OPENSSL_strlcpy(buf, (char *)userdata, (size_t)size);
    return 1;
}

/*
 * Test wrapping old style PEM password callback in a UI method through the
 * use of UI utility functions
 */
static int test_old()
{
    UI_METHOD *ui_method = NULL;
    UI *ui = NULL;
    char defpass[] = "password";
    char pass[16];
    int ok = 0;

    if ((ui_method =
         UI_UTIL_wrap_read_pem_callback(test_pem_password_cb, 0)) == NULL
        || (ui = UI_new_method(ui_method)) == NULL)
        goto err;

    /* The wrapper passes the UI userdata as the callback userdata param */
    UI_add_user_data(ui, defpass);

    if (!UI_add_input_string(ui, "prompt", UI_INPUT_FLAG_DEFAULT_PWD,
                             pass, 0, sizeof(pass) - 1))
        goto err;

    switch (UI_process(ui)) {
    case -2:
        BIO_printf(bio_err, "test_old: UI process interrupted or cancelled\n");
        /* fall through */
    case -1:
        goto err;
    default:
        break;
    }

    if (strcmp(pass, defpass) == 0)
        ok = 1;
    else
        BIO_printf(bio_err, "test_old: password failure\n");

 err:
    if (!ok)
        ERR_print_errors_fp(stderr);
    UI_free(ui);
    UI_destroy_method(ui_method);

    return ok;
}

/* Test of UI.  This uses the UI method defined in apps/apps.c */
static int test_new_ui()
{
    PW_CB_DATA cb_data = {
        "password",
        "prompt"
    };
    char pass[16];
    int ok = 0;

    setup_ui_method();
    if (password_callback(pass, sizeof(pass), 0, &cb_data) > 0
        && strcmp(pass, cb_data.password) == 0)
        ok = 1;
    else
        BIO_printf(bio_err, "test_new: password failure\n");

    if (!ok)
        ERR_print_errors_fp(stderr);

    destroy_ui_method();
    return ok;
}

int test_main(int argc, char *argv[])
{
    int ret;

    bio_err = dup_bio_err(FORMAT_TEXT);

    ADD_TEST(test_old);
    ADD_TEST(test_new_ui);

    ret = run_tests(argv[0]);

    (void)BIO_flush(bio_err);
    BIO_free(bio_err);

    return ret;
}
