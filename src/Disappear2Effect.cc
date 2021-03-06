/*
 * Copyright (C) 2018 Vlad Zagorodniy <vladzzag@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Disappear2Effect.h"

// KConfigSkeleton
#include "disappear2config.h"

// kwineffects
#include <kwinglutils.h>

// Qt
#include <QMatrix4x4>

namespace {
const int Disappear2WindowRole = 0x22A98400;
}

Disappear2Effect::Disappear2Effect()
{
    initConfig<Disappear2Config>();
    reconfigure(ReconfigureAll);

    connect(KWin::effects, &KWin::EffectsHandler::windowAdded,
        this, &Disappear2Effect::markWindow);
    connect(KWin::effects, &KWin::EffectsHandler::windowClosed,
        this, &Disappear2Effect::start);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted,
        this, &Disappear2Effect::stop);

    const auto windows = KWin::effects->stackingOrder();
    for (KWin::EffectWindow* w : windows) {
        markWindow(w);
    }
}

Disappear2Effect::~Disappear2Effect()
{
}

void Disappear2Effect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags);

    Disappear2Config::self()->read();
    m_blacklist = Disappear2Config::blacklist().toSet();
    m_duration = animationTime(Disappear2Config::duration() > 0
            ? Disappear2Config::duration()
            : 160);
    m_opacity = Disappear2Config::opacity();
    m_pitch = Disappear2Config::pitch();
    m_distance = Disappear2Config::distance();
}

void Disappear2Effect::prePaintScreen(KWin::ScreenPrePaintData& data, int time)
{
    auto it = m_animations.begin();
    while (it != m_animations.end()) {
        Timeline& t = it.value();
        t.update(time);
        if (t.done()) {
            KWin::EffectWindow* w = it.key();
            w->unrefWindow();
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }

    if (!m_animations.isEmpty()) {
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }

    KWin::effects->prePaintScreen(data, time);
}

void Disappear2Effect::prePaintWindow(KWin::EffectWindow* w, KWin::WindowPrePaintData& data, int time)
{
    if (m_animations.contains(w)) {
        w->enablePainting(KWin::EffectWindow::PAINT_DISABLED_BY_DELETE);
        data.setTransformed();
    }

    KWin::effects->prePaintWindow(w, data, time);
}

void Disappear2Effect::paintWindow(KWin::EffectWindow* w, int mask, QRegion region, KWin::WindowPaintData& data)
{
    const auto it = m_animations.constFind(w);
    if (it != m_animations.cend()) {
        const qreal t = (*it).value();

        const QRectF screenGeo = KWin::GLRenderTarget::virtualScreenGeometry();
        const QRectF windowGeo = w->geometry();

        // Perspective projection distorts objects near edges
        // of the viewport. This is critical because distortions
        // near edges of the viewport are not desired with this effect.
        // To fix this, the center of the window will be moved to the origin,
        // after applying perspective projection, the center is moved back
        // to its "original" projected position. Overall, this is how the window
        // will be transformed:
        //  [move to the origin] -> [rotate] -> [translate] ->
        //    -> [perspective projection] -> [reverse "move to the origin"]
        const QMatrix4x4 oldProjMatrix = data.screenProjectionMatrix();
        const QVector3D invOffset = oldProjMatrix.map(QVector3D(windowGeo.center()));
        QMatrix4x4 invOffsetMatrix;
        invOffsetMatrix.translate(invOffset.x(), invOffset.y());
        data.setProjectionMatrix(invOffsetMatrix * oldProjMatrix);

        // Move the center of the window to the origin.
        const QPointF offset = screenGeo.center() - windowGeo.center();
        data.translate(offset.x(), offset.y());

        data.setRotationAxis(Qt::XAxis);
        data.setRotationOrigin(QVector3D(0, w->height(), 0));
        data.setRotationAngle(interpolate(0, m_pitch, t));
        data.setZTranslation(interpolate(0, m_distance, t));
        data.multiplyOpacity(interpolate(1, m_opacity, t));
    }

    KWin::effects->paintWindow(w, mask, region, data);
}

void Disappear2Effect::postPaintScreen()
{
    if (!m_animations.isEmpty()) {
        KWin::effects->addRepaintFull();
    }

    KWin::effects->postPaintScreen();
}

bool Disappear2Effect::isActive() const
{
    return !m_animations.isEmpty();
}

bool Disappear2Effect::supported()
{
    return KWin::effects->isOpenGLCompositing()
        && KWin::effects->animationsSupported();
}

bool Disappear2Effect::shouldAnimate(const KWin::EffectWindow* w) const
{
    if (KWin::effects->activeFullScreenEffect()) {
        return false;
    }

    const auto* closeGrab = w->data(KWin::WindowClosedGrabRole).value<void*>();
    if (closeGrab != nullptr && closeGrab != this) {
        return false;
    }

    if (m_blacklist.contains(w->windowClass())) {
        return false;
    }

    if (w->data(Disappear2WindowRole).toBool()) {
        return true;
    }

    if (!w->isManaged()) {
        return false;
    }

    return w->isNormalWindow()
        || w->isDialog();
}

void Disappear2Effect::start(KWin::EffectWindow* w)
{
    if (!shouldAnimate(w)) {
        return;
    }

    // Tell other effects(like fade, for example) to ignore this window.
    w->setData(KWin::WindowClosedGrabRole, QVariant::fromValue(static_cast<void*>(this)));

    w->refWindow();

    Timeline& t = m_animations[w];
    t.setDuration(m_duration);
    t.setEasingCurve(QEasingCurve::OutCurve);
}

void Disappear2Effect::stop(KWin::EffectWindow* w)
{
    m_animations.remove(w);
}

void Disappear2Effect::markWindow(KWin::EffectWindow* w)
{
    if (!shouldAnimate(w)) {
        return;
    }
    w->setData(Disappear2WindowRole, true);
}
