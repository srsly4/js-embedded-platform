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

void module_http_platform_request_thread_wait(https_request_sess_t *session) {
    uint32_t ulNotifiedValue;
    xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY);
}

void module_http_platform_request_thread_notify(https_request_sess_t *notified_session) {
    xTaskNotify(notified_session->platform_data, 0, eNoAction);
}

void module_http_platform_start_request_thread(https_request_sess_t *session) {
    TaskHandle_t task;
    xTaskCreate(_request_thread, "http_req_task", 512, session, 3, &task);
    session->platform_data = task;
}
void module_http_platform_stop_request_thread(https_request_sess_t *session) {
    vTaskDelete(session->platform_data);
}

