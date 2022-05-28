//
//  linked_list.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/5/2022
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// q&d linked list

struct list_elem {
	struct list_elem *next;
	size_t thing_size;
	void *thing;
};

struct list {
	struct list_elem *first;
	struct list_elem *head;
};

#define LIST_INITIALIZER (struct list){ .first = NULL, .head = NULL }

static inline struct list_elem *_list_find_head(struct list *list) {
	if (!list->first) return NULL;
	struct list_elem *head = list->first;
	while (head->next) head = head->next;
	return head;
}

static inline void elem_destroy(struct list_elem *elem) {
	if (elem->next) elem_destroy(elem->next);
	if (elem->thing) free(elem->thing);
	free(elem);
}

static inline void list_destroy(struct list *list) {
	if (!list->first) return;
	struct list_elem *head = list->first;
	if (head) elem_destroy(head);
	list->first = NULL;
}

static inline bool list_empty(struct list *list) {
	return !_list_find_head(list);
}

static inline size_t list_elems(struct list *list) {
	size_t elems = 0;
	struct list_elem *head = list->first;
	while (head) {
		elems++;
		head = head->next;
	}
	return elems;
}

static inline struct list_elem *list_new_elem(const void *thing, size_t thing_size) {
	struct list_elem *elem = calloc(1, sizeof(*elem));
	elem->thing_size = thing_size;
	elem->thing = calloc(1, elem->thing_size);
	memcpy(elem->thing, thing, elem->thing_size);
	return elem;
}

static inline struct list_elem *_list_append(struct list *list, const void *thing, size_t thing_size) {
	if (!list) return NULL;
	if (!thing) return NULL;
	if (!thing_size) return NULL;
	if (!list->first) {
		list->first = list_new_elem(thing, thing_size);
		list->head = list->first;
		return list->first;
	}
	struct list_elem *head = list->head;
	head->next = list_new_elem(thing, thing_size);
	list->head = head->next;
	return head->next;
}

#define list_append(list, thing) _list_append(&list, &thing, sizeof(thing))

static inline void list_remove_cb(struct list *list, bool (*check_cb)(void *elem)) {
	struct list_elem *current = list->first;
	struct list_elem *prev = current;
	struct list_elem *next = NULL;
	while (current) {
		next = current->next;
		if (check_cb(current->thing)) {
			if (current == list->head) list->head = prev;
			prev->next = current->next;
			if (current == list->first) {
				list->first = current->next;
			}
			if (current->thing) free(current->thing);
			free(current);
			return;
		}
		prev = current;
		current = next;
	}
}

static inline void list_foreach(struct list *list, void (*callback)(void *elem)) {
	struct list_elem *current = list->first;
	struct list_elem *next = NULL;
	while (current) {
		next = current->next;
		callback(current->thing);
		current = next;
	}
}

#define list_foreach_ro(element, list) for (element = list.first; list.first && element; element = element->next)
