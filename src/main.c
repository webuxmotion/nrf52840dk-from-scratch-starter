#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static struct bt_conn *default_conn;
static bt_addr_le_t target_addr;
static volatile bool found_target = false;

static struct bt_gatt_discover_params discover_params;

/* Колбек, який виведе ВСІ доступні атрибути пульта */
static uint8_t discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (!attr) {
		printk("[GATT] Full Discovery finished.\n");
		return BT_GATT_ITER_STOP;
	}

	char uuid_str[BT_UUID_STR_LEN];
	bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));

	/* Виводимо кожен знайдений UUID та його Handle в консоль */
	printk("[GATT ATTR] Handle: 0x%04X | UUID: %s\n", attr->handle, uuid_str);

	return BT_GATT_ITER_CONTINUE;
}

static bool parse_ad_cb(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = (bt_addr_le_t *)user_data;
	char addr_str[BT_ADDR_LE_STR_LEN];
	
	if (!data || !data->data || data->data_len == 0) {
		return true;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (data->type == BT_DATA_NAME_COMPLETE || data->type == BT_DATA_NAME_SHORTENED) {
		char name_buf[32] = {0};
		size_t copy_len = (data->data_len < sizeof(name_buf) - 1) ? data->data_len : sizeof(name_buf) - 1;
		memcpy(name_buf, data->data, copy_len);

		if (strstr(name_buf, "ExpressLRS") != NULL || strstr(name_buf, "Joystick") != NULL) {
			printk("[SCAN] !!! TARGET MATCHED !!! Connecting to %s...\n", name_buf);
			bt_addr_le_copy(&target_addr, addr);
			found_target = true;
			return false; 
		}
	}
	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
	if (found_target) {
		return;
	}
	bt_data_parse(ad, parse_ad_cb, (void *)addr);

	if (found_target) {
		bt_le_scan_stop();
		int err = bt_conn_le_create(&target_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &default_conn);
		if (err) {
			found_target = false;
			bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("[BLE] Connection failed (err 0x%02x)\n", err);
		found_target = false;
		bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	} else {
		printk("[BLE] SUCCESS!!! Connected to RadioMaster TX12 Joystick!\n");

		/* Налаштовуємо ПОВНЕ сканування всіх характеристик (DISCOVER_ATTRIBUTE) */
		discover_params.uuid = NULL; /* NULL означає шукати ВСЕ підряд */
		discover_params.func = discover_cb;
		discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
		discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
		discover_params.type = BT_GATT_DISCOVER_ATTRIBUTE;

		int disc_err = bt_gatt_discover(conn, &discover_params);
		if (disc_err) {
			printk("[GATT] Discovery failed to start (err %d)\n", disc_err);
		} else {
			printk("[GATT] Full Discovery started...\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("[BLE] Disconnected (reason 0x%02x). Re-scanning...\n", reason);
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	found_target = false;
	bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;
	k_msleep(500);

	printk("\n=========================================\n");
	printk("   nRF52840 DK <-> TX12 GATT Explorer    \n");
	printk("=========================================\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth Stack Initialized.\n");
	bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);

	while (1) {
		k_msleep(1000);
	}
	return 0;
}
