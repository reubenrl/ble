
#include "custom_adv.h"
#include "native_gecko.h"

#include <string.h>

void fill_adv_packet(tsCustomAdv *pData, uint8 flags, uint16 companyID, uint8 num_presses, uint8 last_press, char *name)
{
	int n;

	pData->len_flags = 0x02;
	pData->type_flags = 0x01;
	pData->val_flags = flags;

	pData->len_manuf = 5;  /* 1+2+2 bytes for type, company ID and the payload */
	pData->type_manuf = 0xFF;
	pData->company_LO = companyID & 0xFF;
	pData->company_HI = (companyID >> 8) & 0xFF;

	pData->num_presses = num_presses;
	pData->last_press = last_press;

	n = strlen(name); // name length, excluding null terminator
	if(n > NAME_MAX_LENGTH)
	{
		pData->type_name = 0x08; // incomplete name
	}
	else
	{
		pData->type_name = 0x09;
	}

	strncpy(pData->name, name, NAME_MAX_LENGTH);

	if(n > NAME_MAX_LENGTH)
	{
		n = NAME_MAX_LENGTH;
	}

	pData->len_name = 1+n; /* length of name element is the name string length + 1 for the AD type */

	/* calculate total length of advertising data*/
	pData->data_size = 3 + (1 + pData->len_manuf) + (1 + pData->len_name);
}

void start_adv(tsCustomAdv *pData)
{
	/* set custom advertising payload */
	gecko_cmd_le_gap_set_adv_data(0, pData->data_size, (const uint8 *)pData);

	/* start advertising using custom data */
	gecko_cmd_le_gap_set_mode(le_gap_user_data, le_gap_undirected_connectable);
}

void update_adv_data(tsCustomAdv *pData, uint8 num_presses, uint8 last_press)
{

	/* update the two variable fields in the custom advertising packet */
	pData->num_presses = num_presses;
	pData->last_press = last_press;

	/* set custom advertising payload */
	gecko_cmd_le_gap_set_adv_data(0, pData->data_size, (const uint8 *)pData);

	/* note: no need to call gecko_cmd_le_gap_set_mode again here. */
}
