//
//  node_parse.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 22/11/2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>

struct cJSON;

struct cr_value_node *cr_value_node_build(const struct cJSON *node);
void cr_value_node_free(struct cr_value_node *d);

struct cr_color_node *cr_color_node_build(const struct cJSON *desc);
void cr_color_node_free(struct cr_color_node *d);

struct cr_vector_node *cr_vector_node_build(const struct cJSON *node);
void cr_vector_node_free(struct cr_vector_node *d);

struct cr_shader_node *cr_shader_node_build(const struct cJSON *node);
void cr_shader_node_free(struct cr_shader_node *d);

// TODO: Throw these extras in a separate file
struct color color_parse(const struct cJSON *data);
