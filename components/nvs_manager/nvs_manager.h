#ifndef WIFI_NVS_MANAGER_H
#define WIFI_NVS_MANAGER_H

#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define APINFO_TOTAL_LEN 700
#define AP_RECORD_SIZE 140
#define MAX_AP_COUNT (APINFO_TOTAL_LEN / AP_RECORD_SIZE)

    /**
     * @brief Cấu trúc lưu thông tin một mạng Wi-Fi đã lưu.
     */
    typedef struct
    {
        char ssid[32]; /*!< SSID của Access Point */
        char pass[65]; /*!< Mật khẩu Wi-Fi */
    } saved_ap_t;

    void nvs_init();
    /**
     * @brief Liệt kê toàn bộ namespace hiện có trong NVS.
     *
     * Dùng cho mục đích debug, xem các namespace (ví dụ: nvs.net80211, storage, wifi_cfg...).
     */
    void nvs_list_all_namespaces(void);

    /**
     * @brief Liệt kê toàn bộ key và giá trị trong một namespace cụ thể.
     *
     * @param namespace_name Tên namespace cần đọc (ví dụ: "nvs.net80211")
     */
    void nvs_list_keys_in_namespace(const char *namespace_name);

    /**
     * @brief Đọc tất cả thông tin Wi-Fi đã lưu trong `nvs.net80211` → “sta.apinfo”.
     *
     * @param ap_list  Mảng để lưu danh sách các AP đã lưu.
     * @param max_ap   Số lượng tối đa cần đọc (kích thước mảng ap_list).
     * @return int     Số lượng AP đọc được.
     */
    int wifi_nvs_get_all_saved_ap(saved_ap_t *ap_list, int max_ap);

    /**
     * @brief Lấy SSID và Password đầu tiên trong danh sách Wi-Fi đã lưu.
     *
     * @param out_ssid buffer để chứa SSID (ít nhất 33 bytes)
     * @param out_pass buffer để chứa password (ít nhất 65 bytes)
     * @return esp_err_t ESP_OK nếu thành công, lỗi nếu không tìm thấy hoặc không đủ bộ nhớ.
     */
    esp_err_t wifi_nvs_get_sta_credentials(char *out_ssid, char *out_pass);

    /**
     * @brief Ghi một key và giá trị vào namespace chỉ định.
     *
     * @param namespace_name  Tên namespace (tự động tạo nếu chưa có)
     * @param key             Tên key cần lưu
     * @param value_type      Kiểu dữ liệu: NVS_TYPE_I32, NVS_TYPE_STR, NVS_TYPE_BLOB
     * @param value           Con trỏ tới dữ liệu cần ghi
     * @param length          Độ dài dữ liệu (với string có thể = strlen(value)+1)
     * @return esp_err_t      ESP_OK nếu thành công, hoặc mã lỗi nếu thất bại
     */
    esp_err_t nvs_write_key_value(const char *namespace_name,
                                  const char *key,
                                  nvs_type_t value_type,
                                  const void *value,
                                  size_t length);
    /**
     * @brief Xóa một key trong namespace của NVS.
     *
     * @param namespace_name  Tên namespace chứa key cần xóa (ví dụ: "storage", "nvs.net80211", ...)
     * @param key             Tên key cần xóa
     * @return esp_err_t      ESP_OK nếu xóa thành công, hoặc mã lỗi nếu thất bại
     *
     * @note  - Nếu key không tồn tại, hàm trả về ESP_ERR_NVS_NOT_FOUND.
     *         - Sau khi xóa, cần commit để thay đổi có hiệu lực vĩnh viễn.
     */
    esp_err_t nvs_delete_key(const char *namespace_name, const char *key);

    /**
     * @brief Xóa toàn bộ các key trong một namespace của NVS.
     *
     * @param namespace_name  Tên namespace cần xóa toàn bộ dữ liệu (ví dụ: "storage", "nvs.net80211")
     * @return esp_err_t      ESP_OK nếu thành công, hoặc mã lỗi khác nếu thất bại
     *
     * @note  - Namespace vẫn tồn tại trong flash, chỉ là trống rỗng.
     *         - Không thể “xóa” namespace thật sự khỏi flash vì ESP-IDF không hỗ trợ.
     *         - Sau khi xóa, cần gọi `nvs_commit()` để áp dụng thay đổi.
     */
    esp_err_t nvs_erase_namespace(const char *namespace_name);
#ifdef __cplusplus
}
#endif

#endif // WIFI_NVS_MANAGER_H
