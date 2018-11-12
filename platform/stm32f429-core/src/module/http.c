#include <cmsis_os.h>
#include "module/http.h"

void module_http_platform_mutex_lock() {

}

void module_http_platform_mutex_unlock() {

}

void _request_thread(void* args) {
    https_request_sess_t *session = args;
    module_http_request_thread(session);
}

void module_http_platform_start_request_thread(https_request_sess_t *session) {
    TaskHandle_t task;
    xTaskCreate(_request_thread, "http_req_task", 128, session, 3, &task);
    session->platform_data = task;
}
void module_http_platform_stop_request_thread(https_request_sess_t *session) {
    vTaskDelete(session->platform_data);
}

