#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_manager.h"

static const char *TAG = "nvs_manager";

/* ================================================================
 *                    HÀM PHỤ TRỢ CỤC BỘ
 * ================================================================ */

/**
 * @brief Chuyển trường byte sang chuỗi ký tự ASCII có thể in ra.
 */
static void field_to_cstr(const uint8_t *src, size_t max_len,
                          char *dst, size_t dst_len)
{
    size_t j = 0;
    for (size_t i = 0; i < max_len && j + 1 < dst_len; ++i)
    {
        uint8_t b = src[i];
        if (b == 0x00)
            break;
        dst[j++] = (char)(isprint(b) ? b : '?');
    }
    dst[j] = '\0';
}

/**
 * @brief In giá trị mảng byte dưới dạng mã hex (debug).
 */
static void print_ascii_hex_until_nul(const uint8_t *src, size_t max_len)
{
    for (size_t i = 0; i < max_len; ++i)
    {
        if (src[i] == 0x00)
            break;
        printf("%02X ", src[i]);
    }
    printf("\n");
}

/* ================================================================
 *                    CÁC HÀM CHÍNH CÔNG KHAI
 * ================================================================ */
void nvs_init()
{
    esp_log_level_set("nvs_manager", ESP_LOG_NONE);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}
void nvs_list_all_namespaces(void)
{
    printf("=== Liệt kê toàn bộ namespace trong NVS ===\n");
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find("nvs", NULL, NVS_TYPE_ANY, &it);

    while (res == ESP_OK)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        printf("Namespace: %s | Key: %s\n", info.namespace_name, info.key);
        res = nvs_entry_next(&it);
    }

    nvs_release_iterator(it);
    printf("=== Kết thúc liệt kê ===\n");
}

void nvs_list_keys_in_namespace(const char *namespace_name)
{
    nvs_handle_t nvsHandler;
    esp_err_t ret = nvs_open(namespace_name, NVS_READWRITE, &nvsHandler);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Không mở được namespace %s", namespace_name);
        return;
    }

    printf("=== Liệt kê key trong namespace '%s' ===\n", namespace_name);

    nvs_iterator_t it = NULL;
    ret = nvs_entry_find("nvs", namespace_name, NVS_TYPE_ANY, &it);
    while (ret == ESP_OK)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        printf("Key: %-15s | Type: %d\n", info.key, info.type);

        // Hiển thị giá trị tùy theo kiểu dữ liệu
        if (info.type == NVS_TYPE_I32)
        {
            int32_t val;
            if (nvs_get_i32(nvsHandler, info.key, &val) == ESP_OK)
                printf("   -> int32_t value = %ld\n", val);
        }
        else if (info.type == NVS_TYPE_STR)
        {
            size_t len = 0;
            nvs_get_str(nvsHandler, info.key, NULL, &len);
            char *value = malloc(len);
            if (value)
            {
                if (nvs_get_str(nvsHandler, info.key, value, &len) == ESP_OK)
                    printf("   -> string value = %s\n", value);
                free(value);
            }
        }
        else if (info.type == NVS_TYPE_BLOB)
        {
            size_t blob_size = 0;
            nvs_get_blob(nvsHandler, info.key, NULL, &blob_size);
            uint8_t *blob = malloc(blob_size);
            if (blob)
            {
                if (nvs_get_blob(nvsHandler, info.key, blob, &blob_size) == ESP_OK)
                {
                    printf("   -> blob (%d bytes): ", (int)blob_size);
                    for (int i = 0; i < blob_size; i++)
                        printf("%02X ", blob[i]);
                    printf("\n");
                }
                free(blob);
            }
        }

        ret = nvs_entry_next(&it);
    }

    nvs_close(nvsHandler);
    nvs_release_iterator(it);
}

/**
 * @brief Đọc danh sách tất cả mạng Wi-Fi đã lưu trong "nvs.net80211".
 */
int wifi_nvs_get_all_saved_ap(saved_ap_t *ap_list, int max_ap)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("nvs.net80211", NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Không mở được namespace nvs.net80211: %s", esp_err_to_name(err));
        return 0;
    }

    size_t blob_len = 0;
    err = nvs_get_blob(handle, "sta.apinfo", NULL, &blob_len);
    if (err != ESP_OK || blob_len == 0)
    {
        ESP_LOGW(TAG, "Không có key sta.apinfo trong NVS.");
        nvs_close(handle);
        return 0;
    }

    uint8_t *blob = malloc(blob_len);
    if (!blob)
    {
        nvs_close(handle);
        return 0;
    }

    if (nvs_get_blob(handle, "sta.apinfo", blob, &blob_len) != ESP_OK)
    {
        free(blob);
        nvs_close(handle);
        return 0;
    }

    ESP_LOGI(TAG, "Đọc blob sta.apinfo: %d bytes", (int)blob_len);

    int count = 0;
    for (size_t offset = 0; offset + AP_RECORD_SIZE <= blob_len && count < max_ap; offset += AP_RECORD_SIZE)
    {
        uint8_t *entry = blob + offset;

        // Nếu phần đầu toàn 0xFF → kết thúc danh sách
        int all_ff = 1;
        for (int i = 0; i < AP_RECORD_SIZE; i++)
        {
            if (entry[i] != 0xFF)
            {
                all_ff = 0;
                break;
            }
        }
        if (all_ff)
            break;

        uint8_t ssid_len = entry[0];
        if (ssid_len > 32)
            ssid_len = 32;

        const uint8_t *ssid_field = entry + 4;
        const uint8_t *pass_field = entry + 4 + 32;

        field_to_cstr(ssid_field, ssid_len, ap_list[count].ssid, sizeof(ap_list[count].ssid));
        field_to_cstr(pass_field, 65, ap_list[count].pass, sizeof(ap_list[count].pass));

        ESP_LOGI(TAG, "[%d] SSID: %s | PASS: %s", count + 1, ap_list[count].ssid, ap_list[count].pass);
        count++;
    }

    free(blob);
    nvs_close(handle);
    return count;
}

/**
 * @brief Lấy SSID và Password đang sử dụng trong NVS .
 */
esp_err_t wifi_nvs_get_sta_credentials(char *out_ssid, char *out_pass)
{
    if (!out_ssid || !out_pass)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open("nvs.net80211", NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Không mở được namespace nvs.net80211: %s", esp_err_to_name(err));
        return err;
    }

    // === Đọc SSID ===
    size_t ssid_len = 0;
    err = nvs_get_blob(handle, "sta.ssid", NULL, &ssid_len);
    if (err != ESP_OK || ssid_len == 0)
    {
        ESP_LOGW(TAG, "Không có key sta.ssid trong NVS.");
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t *ssid_blob = malloc(ssid_len);
    if (!ssid_blob)
    {
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_blob(handle, "sta.ssid", ssid_blob, &ssid_len);
    if (err != ESP_OK)
    {
        free(ssid_blob);
        nvs_close(handle);
        return err;
    }

    // === Đọc Password ===
    size_t pass_len = 0;
    err = nvs_get_blob(handle, "sta.pswd", NULL, &pass_len);
    if (err != ESP_OK || pass_len == 0)
    {
        ESP_LOGW(TAG, "Không có key sta.pswd trong NVS.");
        free(ssid_blob);
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t *pass_blob = malloc(pass_len);
    if (!pass_blob)
    {
        free(ssid_blob);
        nvs_close(handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_blob(handle, "sta.pswd", pass_blob, &pass_len);
    nvs_close(handle);
    if (err != ESP_OK)
    {
        free(ssid_blob);
        free(pass_blob);
        return err;
    }

    // === Chuyển sang chuỗi C ===
    // Cấu trúc blob: [2 byte length][dữ liệu...]
    uint16_t ssid_real_len = *(uint16_t *)ssid_blob;
    uint16_t pass_real_len = *(uint16_t *)pass_blob;

    if (ssid_real_len > 32)
        ssid_real_len = 32;
    if (pass_real_len > 64)
        pass_real_len = 64;

    memcpy(out_ssid, ssid_blob + 4, ssid_real_len);
    out_ssid[ssid_real_len] = '\0';

    memcpy(out_pass, pass_blob, pass_real_len);
    out_pass[pass_real_len] = '\0';

    ESP_LOGI(TAG, "📡 SSID: %s", out_ssid);
    ESP_LOGI(TAG, "🔑 PASS: %s", out_pass);

    free(ssid_blob);
    free(pass_blob);
    return ESP_OK;
}

esp_err_t nvs_write_key_value(const char *namespace_name,
                              const char *key,
                              nvs_type_t value_type,
                              const void *value,
                              size_t length)

{
    if (!namespace_name || !key || !value)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err;
    nvs_handle_t handle;

    // Mở namespace (tự tạo nếu chưa tồn tại)
    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Không thể mở namespace '%s': %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    switch (value_type)
    {
    case NVS_TYPE_I32:
        if (length != sizeof(int32_t))
        {
            ESP_LOGE(TAG, "Chiều dài không hợp lệ cho kiểu I32");
            nvs_close(handle);
            return ESP_ERR_INVALID_SIZE;
        }
        err = nvs_set_i32(handle, key, *(int32_t *)value);
        break;

    case NVS_TYPE_STR:
        err = nvs_set_str(handle, key, (const char *)value);
        break;

    case NVS_TYPE_BLOB:
        err = nvs_set_blob(handle, key, value, length);
        break;

    default:
        ESP_LOGE(TAG, "Kiểu dữ liệu không được hỗ trợ: %d", value_type);
        nvs_close(handle);
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
        if (err == ESP_OK)
            ESP_LOGI(TAG, "Đã ghi key='%s' vào namespace='%s' thành công.", key, namespace_name);
        else
            ESP_LOGE(TAG, "Lỗi commit: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGE(TAG, "Lỗi ghi giá trị: %s", esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_delete_key(const char *namespace_name, const char *key)
{
    if (!namespace_name || !key)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Không mở được namespace '%s': %s", namespace_name, esp_err_to_name(err));
        return err;
    }

    // Xóa key
    err = nvs_erase_key(handle, key);
    if (err == ESP_OK)
    {
        // Ghi thay đổi xuống flash
        esp_err_t commit_err = nvs_commit(handle);
        if (commit_err == ESP_OK)
        {
            ESP_LOGI(TAG, "Đã xóa key='%s' khỏi namespace='%s'", key, namespace_name);
        }
        else
        {
            ESP_LOGE(TAG, "Lỗi commit khi xóa key='%s': %s", key, esp_err_to_name(commit_err));
            err = commit_err;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Key '%s' không tồn tại trong namespace '%s'", key, namespace_name);
    }
    else
    {
        ESP_LOGE(TAG, "Không thể xóa key='%s': %s", key, esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_erase_namespace(const char *namespace_name)
{
    if (!namespace_name)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Không mở được namespace '%s': %s",
                 namespace_name, esp_err_to_name(err));
        return err;
    }

    // Xóa toàn bộ key trong namespace
    err = nvs_erase_all(handle);
    if (err == ESP_OK)
    {
        esp_err_t commit_err = nvs_commit(handle);
        if (commit_err == ESP_OK)
        {
            ESP_LOGI(TAG, "✅ Đã xóa toàn bộ dữ liệu trong namespace '%s'", namespace_name);
        }
        else
        {
            ESP_LOGE(TAG, "Lỗi commit sau khi xóa namespace '%s': %s",
                     namespace_name, esp_err_to_name(commit_err));
            err = commit_err;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Không thể xóa namespace '%s': %s",
                 namespace_name, esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}
