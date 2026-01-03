/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Drawable.h"
#include <Assets/AssetManager.h>
#include <UI/UITheme.h>

namespace UI {

void Drawable::preparePlaceholder(RenderBuffer* buffer)
{
    auto& renderer = the_renderer();
    switch (style->type) {
    case DrawableType::None: {
        return; // Nothing to do!
    } break;

    case DrawableType::Color: {
        placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
    } break;

    case DrawableType::Gradient: {
        placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
    } break;

    case DrawableType::Ninepatch: {
        placeholder = appendDrawNinepatchPlaceholder(buffer, style->ninepatch.get().ninepatch.texture, renderer.shaderIds.textured);
    } break;

    case DrawableType::Sprite: {
        placeholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.pixelArt, true);
    } break;

        INVALID_DEFAULT_CASE;
    }
}

void Drawable::fillPlaceholder(Rect2I bounds)
{
    switch (style->type) {
    case DrawableType::None: {
        return; // Nothing to do!
    } break;

    case DrawableType::Color: {
        fillDrawRectPlaceholder(&placeholder.get<DrawRectPlaceholder>(), bounds, style->color);
    } break;

    case DrawableType::Gradient: {
        fillDrawRectPlaceholder(&placeholder.get<DrawRectPlaceholder>(), bounds, style->gradient.color00, style->gradient.color01, style->gradient.color10, style->gradient.color11);
    } break;

    case DrawableType::Ninepatch: {
        fillDrawNinepatchPlaceholder(&placeholder.get<DrawNinepatchPlaceholder>(), bounds, &style->ninepatch.get().ninepatch, style->color);
    } break;

    case DrawableType::Sprite: {
        fillDrawRectPlaceholder(&placeholder.get<DrawRectPlaceholder>(), bounds, &style->sprite.get(), style->color);
    } break;

        INVALID_DEFAULT_CASE;
    }
}

void Drawable::draw(RenderBuffer* buffer, Rect2I bounds)
{
    auto& renderer = the_renderer();
    switch (style->type) {
    case DrawableType::None: {
        return; // Nothing to do!
    } break;

    case DrawableType::Color: {
        drawSingleRect(buffer, bounds, renderer.shaderIds.untextured, style->color);
    } break;

    case DrawableType::Gradient: {
        drawSingleRect(buffer, bounds, renderer.shaderIds.untextured, style->gradient.color00, style->gradient.color01, style->gradient.color10, style->gradient.color11);
    } break;

    case DrawableType::Ninepatch: {
        drawNinepatch(buffer, bounds, renderer.shaderIds.textured, &style->ninepatch.get().ninepatch, style->color);
    } break;

    case DrawableType::Sprite: {
        drawSingleSprite(buffer, &style->sprite.get(), bounds, renderer.shaderIds.textured, style->color);
    } break;

        INVALID_DEFAULT_CASE;
    }
}

}
