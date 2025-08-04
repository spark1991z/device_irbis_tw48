// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Touchscreen driver DMI based configuration code
 *
 * Copyright (c) 2017 Red Hat Inc.
 *
 * Red Hat authors:
 * Hans de Goede <hdegoede@redhat.com>
 */

#include <linux/acpi.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/efi_embedded_fw.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kstrtox.h>
#include <linux/notifier.h>
#include <linux/property.h>
#include <linux/string.h>

struct ts_dmi_data {
	/* The EFI embedded-fw code expects this to be the first member! */
	struct efi_embedded_fw_desc embedded_fw;
	const char *acpi_name;
	const struct property_entry *properties;
};

/* NOTE: Please keep all entries sorted alphabetically */

// -----------------------------------------------------------------------------------
// IRBIS TW48 DMI BEGIN
static const struct property_entry irbis_tw48_props[] = {
	PROPERTY_ENTRY_U32("touchscreen-size-x", 1975),
	PROPERTY_ENTRY_U32("touchscreen-size-y", 1515),
	PROPERTY_ENTRY_BOOL("touchscreen-inverted-y"),
	PROPERTY_ENTRY_BOOL("touchscreen-swapped-x-y"),
	PROPERTY_ENTRY_STRING("firmware-name",/* /usr/lib/firmware/silead/ */ "gsl1680-irbis_tw48.fw"),
	PROPERTY_ENTRY_U32("silead,max-fingers", 10),
	PROPERTY_ENTRY_BOOL("silead,home-button"),
	{ }
};

static const struct ts_dmi_data irbis_tw48_data = {
	.acpi_name	= "MSSL1680:00",
	.properties	= irbis_tw48_props,
};
// IRBIS TW48 DMI END
// -----------------------------------------------------------------------------------

/* NOTE: Please keep this table sorted alphabetically */
const struct dmi_system_id touchscreen_dmi_table[] = {

	/* IRBIS TW48 DMI MATCH BEGIN */
	{
		.driver_data = (void *)&irbis_tw48_data,
		.matches = {
			DMI_MATCH(DMI_BIOS_VERSION, "_VERSION_"),
		},
	},
	/* IRBIS TW48 DMI MATCH END */
	{ }
};

static struct ts_dmi_data *ts_data;

static void ts_dmi_add_props(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int error;

	if (has_acpi_companion(dev) &&
	    strstarts(client->name, ts_data->acpi_name)) {
		error = device_create_managed_software_node(dev, ts_data->properties, NULL);
		if (error)
			dev_err(dev, "failed to add properties: %d\n", error);
	}
}

static int ts_dmi_notifier_call(struct notifier_block *nb,
				unsigned long action, void *data)
{
	struct device *dev = data;
	struct i2c_client *client;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		client = i2c_verify_client(dev);
		if (client)
			ts_dmi_add_props(client);
		break;

	default:
		break;
	}

	return 0;
}

#define MAX_CMDLINE_PROPS 16

static struct property_entry ts_cmdline_props[MAX_CMDLINE_PROPS + 1];

static struct ts_dmi_data ts_cmdline_data = {
	.properties = ts_cmdline_props,
};

static int __init ts_parse_props(char *str)
{
	/* Save the original str to show it on syntax errors */
	char orig_str[256];
	char *name, *value;
	u32 u32val;
	int i, ret;

	strscpy(orig_str, str);

	/*
	 * str is part of the static_command_line from init/main.c and poking
	 * holes in that by writing 0 to it is allowed, as is taking long
	 * lasting references to it.
	 */
	ts_cmdline_data.acpi_name = strsep(&str, ":");

	for (i = 0; i < MAX_CMDLINE_PROPS; i++) {
		name = strsep(&str, ":");
		if (!name || !name[0])
			break;

		/* Replace '=' with 0 and make value point past '=' or NULL */
		value = name;
		strsep(&value, "=");
		if (!value) {
			ts_cmdline_props[i] = PROPERTY_ENTRY_BOOL(name);
		} else if (isdigit(value[0])) {
			ret = kstrtou32(value, 0, &u32val);
			if (ret)
				goto syntax_error;

			ts_cmdline_props[i] = PROPERTY_ENTRY_U32(name, u32val);
		} else {
			ts_cmdline_props[i] = PROPERTY_ENTRY_STRING(name, value);
		}
	}

	if (!i || str)
		goto syntax_error;

	ts_data = &ts_cmdline_data;
	return 1;

syntax_error:
	pr_err("Invalid '%s' value for 'i2c_touchscreen_props='\n", orig_str);
	return 1; /* "i2c_touchscreen_props=" is still a known parameter */
}
__setup("i2c_touchscreen_props=", ts_parse_props);

static struct notifier_block ts_dmi_notifier = {
	.notifier_call = ts_dmi_notifier_call,
};

static int __init ts_dmi_init(void)
{
	const struct dmi_system_id *dmi_id;
	struct ts_dmi_data *ts_data_dmi;
	int error;

	dmi_id = dmi_first_match(touchscreen_dmi_table);
	ts_data_dmi = dmi_id ? dmi_id->driver_data : NULL;

	if (ts_data) {
		/*
		 * Kernel cmdline provided data takes precedence, copy over
		 * DMI efi_embedded_fw info if available.
		 */
		if (ts_data_dmi)
			ts_data->embedded_fw = ts_data_dmi->embedded_fw;
	} else if (ts_data_dmi) {
		ts_data = ts_data_dmi;
	} else {
		return 0; /* Not an error */
	}

	/* Some dmi table entries only provide an efi_embedded_fw_desc */
	if (!ts_data->properties)
		return 0;

	error = bus_register_notifier(&i2c_bus_type, &ts_dmi_notifier);
	if (error)
		pr_err("%s: failed to register i2c bus notifier: %d\n",
			__func__, error);

	return error;
}

/*
 * We are registering out notifier after i2c core is initialized and i2c bus
 * itself is ready (which happens at postcore initcall level), but before
 * ACPI starts enumerating devices (at subsys initcall level).
 */
arch_initcall(ts_dmi_init);
