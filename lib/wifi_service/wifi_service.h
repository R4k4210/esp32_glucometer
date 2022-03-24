#include "esp_system.h"
#include "esp_event.h"

class WifiService {
    private:
        void init();
        static esp_err_t event_handler(void *ctx, system_event_t *event);
    public:
        void scan();
};