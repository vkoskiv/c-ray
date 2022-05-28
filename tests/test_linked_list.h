//
//  test_linked_list.h
//  C-ray
//
//  Created by Valtteri on 28.5.2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/utils/linked_list.h"

#define ATTEMPTS 1024

bool llist_basic(void) {
	struct list list = LIST_INITIALIZER;
	test_assert(list_empty(&list));

	for (size_t i = 0; i < 1000; ++i) {
		list_append(list, i);
	}

	test_assert(list_elems(&list) == 1000);

	list_destroy(&list);

	return true;
}

bool check_fn(void *elem) {
	size_t *val = (size_t *)elem;
	return *val == 5;
}

bool llist_remove_cb(void) {
	struct list list = LIST_INITIALIZER;
	test_assert(list_empty(&list));

	for (size_t i = 0; i < 10; ++i) {
		list_append(list, i);
	}
	test_assert(list_elems(&list) == 10);

	list_remove_cb(&list, &check_fn);

	test_assert(list_elems(&list) == 9);

	struct list_elem *elem = NULL;
	list_foreach_ro(elem, list) {
		test_assert(elem->thing_size == sizeof(size_t));
		size_t *val = (size_t *)elem->thing;
		test_assert(*val != 5);
	}

	list_remove_cb(&list, &check_fn);
	list_remove_cb(&list, &check_fn);
	list_remove_cb(&list, &check_fn);
	test_assert(list_elems(&list) == 9);

	list_destroy(&list);

	return true;
}

bool true_fn(void *elem) {
	(void)elem;
	return true;
}

bool llist_remove_after_empty(void) {
	struct list list = LIST_INITIALIZER;
	test_assert(list_empty(&list));
	for (size_t i = 0; i < 10; ++i) {
		list_append(list, i);
	}
	for (size_t i = 10; i > 0; i--) {
		test_assert(list_elems(&list) == i);
		list_remove_cb(&list, &true_fn);
	}
	test_assert(list_elems(&list) == 0);
	list_remove_cb(&list, &true_fn);
	test_assert(list_elems(&list) == 0);
	list_remove_cb(&list, &true_fn);
	test_assert(list_elems(&list) == 0);
	list_remove_cb(&list, &true_fn);
	list_destroy(&list);
	return true;
}

