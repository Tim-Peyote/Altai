# Altai UI Style Guide

Target specification, updated 2026-09-08. Not a runtime QA report. Visual palette and material direction: [ART_DIRECTION](../art_sources/ART_DIRECTION.md). Interaction requirements: [UX_INTERACTION](UX_INTERACTION.md).

## Intent

The interface must protect the landscape. Permanent HUD occupies the outer safe zones; the centre belongs to navigation, interaction and metamorphosis. The visual language is restrained field instrumentation with Altai expedition materials, not a generic neon dashboard.

Composition principles:

- Use restrained colours, tactile materials and clear item presentation. Combine a left item browser, central specimen or equipment preview and right detail panel. See [inventory composition and proposed UE implementation](INVENTORY_PRESENTATION.md).
- Keep information in small separated groups with generous negative space. Show stamina and environment information when relevant.
- Present survival resources and food as one coherent preparation model.
- Keep exploration visually quiet; contextual prompts and warnings become prominent when needed.

## Runtime HUD

- Safe horizontal margin: `clamp(viewport_width * 1.9%, 16 px, 34 px)`.
- The current objective appears briefly after an update in the top-left and can be recalled; it does not remain as a permanent quest panel. When visible, the objective lives top-left; time and weather top-right; current danger may occupy the top-centre lane.
- Health and stamina form one compact bottom-left cluster. Normal temperature, zero wetness, zero spores and zero toxicity are hidden. Food slots are hidden while empty.
- Tool and distraction counts use typography and shadow, not independent framed cards.
- Interaction stays centred near the lower third and appears only for a valid target.
- Important changes use a short one-shot notice. Persistent prose must not remain over the reticle.

## Modal screens

- Author the inventory layout in `WBP_Inventory` and its optional 3D presentation in a separately openable `L_Preview_Inventory` with a reusable preview rig. These are proposed assets. Collapse the central preview on narrow layouts before compromising item actions or readable text.
- Inventory reduces grid columns as width decreases. Filters wrap; sorting remains accessible, and item actions never disappear. Detail scrolling must not swallow use/equip/drop controls. Validate at 1280×720, 800×600 and 640×600 rather than assuming fixed breakpoints guarantee usability.
- Outer modal margin is proportional and clamped. Content padding is larger than control-to-control spacing.
- Selected state is expressed by one bright contour and tonal lift. Unselected cards use low-opacity surfaces without heavy drop shadows.
- Text hierarchy: screen title, navigation, capacity summary, content, contextual hint. Avoid repeated section labels when position already conveys meaning.
- Journal is clamped to viewport dimensions rather than relying on a fixed 1080×620 rectangle.

## Motion and accessibility

- Transitions stay short (`0.16–0.24 s`) and never delay control.
- Critical state cannot be communicated by colour alone: every hazard has text and a meter/state label.
- Minimum interactive height is 34 px in dense modal navigation and 42 px for isolated interaction chips.
- All screens retain keyboard/gamepad focus states and must fit the 1280×720 reference viewport.
