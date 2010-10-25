/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2010 WiredTiger, Inc.
 *	All rights reserved.
 *
 * $Id$
 */

#include "wt_internal.h"

/*
 * __wt_bt_build_verify --
 *	Verify the Btree build itself.
 */
int
__wt_bt_build_verify(void)
{
	static const struct {
		char *name;
		u_int size, expected;
	} size_check[] = {
		{ "WT_COL", sizeof(WT_COL), WT_COL_SIZE },
		{ "WT_ITEM", sizeof(WT_ITEM), WT_ITEM_SIZE },
		{ "WT_OFF", sizeof(WT_OFF), WT_OFF_SIZE },
		{ "WT_OVFL", sizeof(WT_OVFL), WT_OVFL_SIZE },
		{ "WT_PAGE_DESC", sizeof(WT_PAGE_DESC), WT_PAGE_DESC_SIZE },
		{ "WT_PAGE_HDR", sizeof(WT_PAGE_HDR), WT_PAGE_HDR_SIZE },
		{ "WT_ROW", sizeof(WT_ROW), WT_ROW_SIZE }
	};
	static const struct {
		char *name;
		u_int size, align;
	} align_check[] = {
		{ "WT_OFF", sizeof(WT_OFF), sizeof(u_int32_t) },
		{ "WT_OVFL", sizeof(WT_OVFL), sizeof(u_int32_t) },
		{ "WT_PAGE_HDR", sizeof(WT_PAGE_HDR), sizeof(u_int32_t) },
		{ "WT_TOC_UPDATE", sizeof(WT_TOC_UPDATE), sizeof(u_int32_t) }
	};
	u_int i;

	/*
	 * The compiler had better not have padded our structures -- make
	 * sure the page header structure is exactly what we expect.
	 */
	for (i = 0; i < WT_ELEMENTS(size_check); ++i) {
		if (size_check[i].size == size_check[i].expected)
			continue;
		__wt_api_env_errx(NULL,
		    "WiredTiger build failed, the %s header structure is not "
		    "the correct size (expected %u, got %u)",
		    size_check[i].name,
		    size_check[i].expected, size_check[i].size);
		return (WT_ERROR);
	}

	/* There are also structures that must be aligned correctly. */
	for (i = 0; i < WT_ELEMENTS(align_check); ++i) {
		if (WT_ALIGN(align_check[i].size,
		    align_check[i].align) == align_check[i].size)
			continue;
		__wt_api_env_errx(NULL,
		    "Build verification failed, the %s structure is not"
		    " correctly aligned", align_check[i].name);
		return (WT_ERROR);
	}

	/*
	 * We mix-and-match 32-bit unsigned values and size_t's, mostly because
	 * we allocate and handle 32-bit objects, and lots of the underlying C
	 * library expects size_t values for the length of memory objects.  We
	 * check, just to be sure.
	 */
	if (sizeof(size_t) < sizeof(u_int32_t)) {
		__wt_api_env_errx(NULL, "%s",
		    "Build verification failed, a size_t is smaller than "
		    "4-bytes");
		return (WT_ERROR);
	}

	return (0);
}

/*
 * __wt_bt_data_copy_to_dbt --
 *	Copy a data/length pair into allocated memory in a DBT.
 */
int
__wt_bt_data_copy_to_dbt(DB *db, u_int8_t *data, size_t len, DBT *copy)
{
	ENV *env;

	env = db->env;

	if (copy->data == NULL || copy->mem_size < len)
		WT_RET(__wt_realloc(env, &copy->mem_size, len, &copy->data));
	memcpy(copy->data, data, copy->size = len);

	return (0);
}

/*
 * __wt_bt_set_ff_and_sa_from_offset --
 *	Set the page's first-free and space-available values from an
 *	address positioned one past the last used byte on the page.
 */
inline void
__wt_bt_set_ff_and_sa_from_offset(WT_PAGE *page, u_int8_t *p)
{
	page->first_free = p;
	page->space_avail = page->size - (u_int)(p - (u_int8_t *)page->hdr);
}

/*
 * __wt_page_write_gen_update --
 *	Handle the page's write generation number.
 */
inline int
__wt_page_write_gen_update(WT_PAGE *page, u_int32_t write_gen)
{
	if (page->write_gen != write_gen)
		return (WT_RESTART);

	++page->write_gen;
	WT_MEMORY_FLUSH;
	return (0);
}

/*
 * __wt_bt_hdr_type --
 *	Return a string representing the page type.
 */
const char *
__wt_bt_hdr_type(WT_PAGE_HDR *hdr)
{
	switch (hdr->type) {
	case WT_PAGE_DESCRIPT:
		return ("database descriptor page");
	case WT_PAGE_COL_FIX:
		return ("column fixed-length leaf");
	case WT_PAGE_COL_INT:
		return ("column internal");
	case WT_PAGE_COL_VAR:
		return ("column variable-length leaf");
	case WT_PAGE_DUP_INT:
		return ("duplicate internal");
	case WT_PAGE_DUP_LEAF:
		return ("duplicate leaf");
	case WT_PAGE_OVFL:
		return ("overflow");
	case WT_PAGE_ROW_INT:
		return ("row internal");
	case WT_PAGE_ROW_LEAF:
		return ("row leaf");
	case WT_PAGE_INVALID:
		return ("invalid");
	default:
		break;
	}
	return ("unknown");
}

/*
 * __wt_bt_item_type --
 *	Return a string representing the item type.
 */
const char *
__wt_bt_item_type(WT_ITEM *item)
{
	switch (WT_ITEM_TYPE(item)) {
	case WT_ITEM_KEY:
		return ("key");
	case WT_ITEM_KEY_OVFL:
		return ("key-overflow");
	case WT_ITEM_DUPKEY:
		return ("duplicate-key");
	case WT_ITEM_DUPKEY_OVFL:
		return ("duplicate-key-overflow");
	case WT_ITEM_DATA:
		return ("data");
	case WT_ITEM_DATA_OVFL:
		return ("data-overflow");
	case WT_ITEM_DUP:
		return ("duplicate");
	case WT_ITEM_DUP_OVFL:
		return ("duplicate-overflow");
	case WT_ITEM_DEL:
		return ("deleted");
	case WT_ITEM_OFF:
		return ("off-page");
	default:
		break;
	}
	return ("unknown");
}
