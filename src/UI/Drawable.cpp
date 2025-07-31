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
        rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
    } break;

    case DrawableType::Gradient: {
        rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.untextured, false);
    } break;

    case DrawableType::Ninepatch: {
        ninepatchPlaceholder = appendDrawNinepatchPlaceholder(buffer, getAsset(&style->ninepatch)->ninepatch.texture, renderer.shaderIds.textured);
    } break;

    case DrawableType::Sprite: {
        rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer.shaderIds.pixelArt, true);
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
        fillDrawRectPlaceholder(&rectPlaceholder, bounds, style->color);
    } break;

    case DrawableType::Gradient: {
        fillDrawRectPlaceholder(&rectPlaceholder, bounds, style->gradient.color00, style->gradient.color01, style->gradient.color10, style->gradient.color11);
    } break;

    case DrawableType::Ninepatch: {
        fillDrawNinepatchPlaceholder(&ninepatchPlaceholder, bounds, &getAsset(&style->ninepatch)->ninepatch, style->color);
    } break;

    case DrawableType::Sprite: {
        fillDrawRectPlaceholder(&rectPlaceholder, bounds, getSprite(&style->sprite), style->color);
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
        drawNinepatch(buffer, bounds, renderer.shaderIds.textured, &getAsset(&style->ninepatch)->ninepatch, style->color);
    } break;

    case DrawableType::Sprite: {
        drawSingleSprite(buffer, getSprite(&style->sprite), bounds, renderer.shaderIds.textured, style->color);
    } break;

        INVALID_DEFAULT_CASE;
    }
}

}
