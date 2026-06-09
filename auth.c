#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "auth.h"

typedef struct {
  const char *login;
  const char *passwd;
} auth_conv_data;

static int talk_conv(int num_msg, const struct pam_message **msg,
               struct pam_response **resp, void *appdata);

static char *handle_pam_error(int pam_ret, pam_handle_t *pamh);

const char *auth_user(const char *login, char *passwd, pam_handle_t **pamh, const char *service){

  auth_conv_data *data = NULL;
  const char *err_msg = NULL;
  int pam_ret = PAM_SUCCESS;

  if (!login || !passwd || !pamh || !service){
    err_msg = "Error: invalid arguments provider to auth_user";
    goto cleanup;
  }

  data = calloc(1, sizeof(auth_conv_data));
  if (!data){
    err_msg = "Error: memory allocation failed";
    goto cleanup;
  }

  data->login = login;
  data->passwd = passwd;

  struct pam_conv conv = { talk_conv, (auth_conv_data *)data };
    
  pam_ret = pam_start(service , login, &conv, pamh);
  if (pam_ret != PAM_SUCCESS){
    err_msg = "Error: pam_start failed";
    goto cleanup;
  }

  pam_ret = pam_authenticate(*pamh, PAM_DISALLOW_NULL_AUTHTOK);
  if (pam_ret != PAM_SUCCESS){
    err_msg = handle_pam_error(pam_ret, *pamh);
    goto cleanup;
  }

  pam_ret = pam_acct_mgmt(*pamh, PAM_DISALLOW_NULL_AUTHTOK);
  if (pam_ret != PAM_SUCCESS){
    err_msg = handle_pam_error(pam_ret, *pamh);
    goto cleanup;
  }
  
cleanup:

  if (passwd){
    size_t len = strlen(passwd);
    if (len > 0){
      memset(passwd, 0, len);
    }
  }
  
  if (err_msg != NULL){
    if (*pamh != NULL){
      pam_end(*pamh, pam_ret);
      *pamh = NULL;
    }
    free(data);
  } 
    return err_msg;
}

int talk_conv(int num_msg, const struct pam_message **msg,
               struct pam_response **resp, void *appdata){

  if (num_msg <= 0 || !msg || !resp || !appdata){
    return PAM_CONV_ERR;
  }

  struct pam_response *rep = calloc(num_msg, sizeof(struct pam_response));
  if (!rep){
    return PAM_CONV_ERR;
  }

  auth_conv_data *ad = (auth_conv_data *)appdata;

  for (int i = 0; i < num_msg; i++){

     if (!msg[i]){
      for (int j = 0; j < i; j++){ free(rep[j].resp); }
      free(rep);
      return PAM_CONV_ERR;
    }

    switch (msg[i]->msg_style){

      case PAM_PROMPT_ECHO_OFF:
        rep[i].resp = strdup(ad->passwd);
        if (!rep[i].resp){
          for (int j = 0; j < i; j++){
            if (rep[j].resp) { free(rep[j].resp); }
          }
          free(rep);
          return PAM_CONV_ERR;
        }
        break;

      case PAM_PROMPT_ECHO_ON:
        rep[i].resp = strdup(ad->login);
        if (!rep[i].resp){
          for (int j = 0; j < i; j++){
            if (rep[j].resp) { free(rep[j].resp); }
          }
          free(rep);
          return PAM_CONV_ERR;
        }
        break;

      case PAM_ERROR_MSG:
      case PAM_TEXT_INFO:
        rep[i].resp = NULL;
        break;

    default:
      for (int j = 0; j < i; j++){
          if (rep[j].resp) { free(rep[j].resp ); }
      }
      free(rep);
      return PAM_CONV_ERR;
    }
  }

  *resp = rep;
  return PAM_SUCCESS;
}

char *handle_pam_error(int ret, pam_handle_t *pamh){
  if (ret != PAM_SUCCESS && ret != PAM_NEW_AUTHTOK_REQD) {
    static char err[256];
    snprintf(err, sizeof(err), "Error: %s", pam_strerror(pamh, ret));
    return err;
  } else {
    return NULL;
  }
}
