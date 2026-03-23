# ⚔️ Building My RPG — Part 3: HP & Mana Vitals System (Unreal Engine 5 GAS)

This repository is part of my ongoing journey building a **modular, AAA-style RPG system in Unreal Engine 5**.

Instead of creating a full game all at once, I’m developing and sharing each core gameplay system individually — focusing on **clean architecture, scalability, and production-level design**.

> 🎯 **Goal:** Build a fully modular RPG framework using industry-standard patterns (GAS, SOLID, reactive systems)

---

## 🧠 TL;DR

A fully reactive HP/Mana UI system built on GAS that:

- Updates **only when attributes change** (no tick polling)
- Uses a **dual-bar trailing system** for smooth combat feedback
- Supports **data-driven stat scaling**
- Is designed to be **plug & play across projects**

---

## 🚀 Why This System Exists

In many projects, UI systems:

- Poll values every frame (wasted performance)
- Are tightly coupled to gameplay logic
- Break under dynamic stat scaling

This system solves those problems by:

- Using **GAS delegates (event-driven updates)**
- Keeping UI fully **decoupled from gameplay systems**
- Supporting **MMO-style proportional stat scaling**

---

## 🎮 Showcase

> ⚠️ Add a GIF here (HP drop + trailing bar effect)

Example:


---

## 🎯 Features

- **Zero-Tick Reactive UI** — Attribute changes drive UI updates via GAS delegates  
- **Smooth Trailing Bars** — WoW/FFXIV-style delayed damage visualization  
- **Data-Driven Stats** — Configurable via `UCharacterDataAsset`  
- **Stat Scaling Pipeline** — Primary → Secondary stat derivation  
- **Fully Modular** — Works with any GAS-based project  
- **Designer-Friendly** — Blueprint-exposed tuning parameters  

---

## 🏗️ Architecture Overview

`[ DATA ] → [ CHARACTER ] → [ GAS ATTRIBUTES ] → [ UI ]`

- **Data Layer** → Defines base stats  
- **Character Layer** → Initializes and applies stats  
- **GAS Layer** → Handles scaling and validation  
- **UI Layer** → Fully reactive (event-driven)  

---

## 🔑 Key Design Decisions

### 1. Event-Driven UI (No Polling)

```cpp
ASC->GetGameplayAttributeValueChangeDelegate(HealthAttr)
    .AddUObject(this, &UPlayerVitalsWidget::OnHealthChanged);
```

### 2. Orchestration Pattern
Validation → Processing → Execution

- Predictable flow
- Easier debugging
- Cleaner separation of concerns

### 3. DRY Stat Scaling

`AdjustAttributeProportionally(...)`

Reusable helper for all stat scaling logic.

### 4. MMO-Style Feedback (Trailing Bars)

- Instant main bar update
- Delayed trailing bar interpolation
- Clear damage readability

---

## 📐 Stat Scaling

| Stat | Effect |
| :--- | :--- |
| **Stamina** | +Max HP |
| **Intellect** | +Max Mana, Spell Damage |
| **Strength** | Attack Damage |
| **Agility** | Cast Speed |

✔ Supports proportional scaling (HP % preserved after stat changes)

---

## 🔌 Integration
Minimal setup:

```cpp
UPlayerVitalsWidget* Widget = CreateWidget<UPlayerVitalsWidget>(PC, WidgetClass);
Widget->AddToViewport();
Widget->InitializeVitals(GetAbilitySystemComponent());
```

---

## 🧩 Part of a Bigger System
This is one piece of a larger RPG architecture I’m building step by step.

**Planned / Upcoming Systems**

- Combat System (Abilities & Damage pipeline)
- Enemy AI (Behavior Trees + GAS integration)
- Inventory & Equipment
- Advanced Stat System (buffs, debuffs, stacking)

---

## 📌 Design Philosophy
- Modularity first → systems should be portable
- Event-driven over polling
- Data-driven over hardcoded
- Scalable from prototype → production

---

## 📄 License
This project is provided as a reference for GAS-based UI systems.
