# ⚔️ Building My RPG — Part 2: HP & Mana Vitals System (Unreal Engine 5 / GAS)
 
This is Part 2 of my ongoing series where I'm building a modular, AAA-style RPG system in Unreal Engine 5 — one system at a time.
Each part is a self-contained, production-quality module. If you missed the earlier parts, I'm sharing them progressively as they're ready.
 
> 🎯 **Goal:** A fully modular RPG framework built on industry-standard patterns — GAS, SOLID principles, and reactive architecture.
 
---
 
## 🧠 TL;DR
 
A fully reactive HP/Mana UI system built on GAS that:
- Updates **only when attributes change** — zero tick polling
- Uses a **dual-bar trailing system** for smooth damage feedback (WoW/FFXIV-style)
- Derives vitals from primary stats via **data-driven scaling**
- Is designed to be **plug & play** across any GAS-based project
 
---
 
## 🚀 The Problem This Solves
 
Most beginner UI systems poll HP every frame with `Tick`. That works until it doesn't — at scale, it wastes CPU, couples UI tightly to gameplay logic, and breaks when stats change dynamically (buffs, level-ups, equipment).
 
This system takes a different approach:
 
| Problem | Solution |
|:---|:---|
| Tick polling | GAS `AttributeValueChangeDelegate` — fires only on actual changes |
| Tight coupling | UI knows nothing about CharacterBase — only about ASC |
| Stat scaling breaks UI | Proportional adjustment preserves current HP% on MaxHP change |
| Hardcoded stats | `UCharacterDataAsset` drives all starting values |
 
---
 
## 🎮 Showcase
 
<div align="center">
  <img src="Gifs/HPMana.gif" width="80%">
  <p><em>HP and Mana bars with GAS-driven trailing effect — updates fire only on attribute change, zero tick polling.</em></p>
</div>

 
---
 
## 🏗️ Architecture
 
```
[ UCharacterDataAsset ]
        ↓
[ ACharacterBase / AHeroCharacter ]  ← Initializes GAS with starting stats
        ↓
[ UCharacterAttributeSet ]           ← Owns all attributes, handles scaling
        ↓
[ UAbilitySystemComponent ]          ← Fires attribute change delegates
        ↓
[ UPlayerVitalsWidget ]              ← Reactive UI, no polling
```
 
Four distinct layers, each with one responsibility. The UI has **zero knowledge** of the character — it only talks to the ASC.
 
---
 
## 🔑 Key Design Decisions
 
### 1. Event-Driven UI — Zero Tick Cost
 
The widget binds to GAS delegates in `InitializeVitals()` and never polls:
 
```cpp
// PlayerVitalsWidget.cpp
CachedASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetHealthAttribute())
    .AddUObject(this, &UPlayerVitalsWidget::OnHealthChanged);
```
 
Delegates are cleaned up properly in `NativeDestruct()` to prevent dangling pointers:
 
```cpp
CachedASC->GetGameplayAttributeValueChangeDelegate(...).RemoveAll(this);
```
 
---
 
### 2. Orchestrator Pattern in `PreAttributeChange`
 
Instead of a giant `if/else` block with inline logic, each scaling rule is delegated to a named helper. The orchestrator just dispatches:
 
```cpp
// CharacterAttributeSet.cpp
if      (Attribute == GetStrengthAttribute())    RecalculateFromStrength(NewValue);
else if (Attribute == GetAgilityAttribute())     RecalculateFromAgility(NewValue);
else if (Attribute == GetStaminaAttribute())     RecalculateFromStamina(NewValue);
else if (Attribute == GetIntellectAttribute())   RecalculateFromIntellect(NewValue);
// ...
```
 
Each helper is self-documenting and testable in isolation. Adding a new stat means adding one new method — nothing else changes.
 
---
 
### 3. Proportional Stat Scaling (DRY)
 
When Stamina increases MaxHealth, the current HP percentage is preserved — not the raw value. This single reusable helper handles both HP and Mana:
 
```cpp
void UCharacterAttributeSet::AdjustAttributeProportionally(
    const FGameplayAttribute& CurrentAttr,
    const FGameplayAttribute& MaxAttr,
    float NewMax, float OldMax)
{
    if (OldMax <= 0.0f || NewMax == OldMax) return;
 
    const float CurrentValue = CurrentAttr.GetNumericValue(this);
    const float NewCurrentValue = CurrentValue * (NewMax / OldMax);
 
    GetOwningAbilitySystemComponent()->SetNumericAttributeBase(CurrentAttr, NewCurrentValue);
}
```
 
So if you have 60/100 HP and gain 50 Stamina (adding 500 MaxHP), you end up at 360/600 — not 60/600.
 
---
 
### 4. Trailing Bar System
 
Two progress bars per vital: one snaps immediately to the target, the other delays — giving the player a clear read on how much damage they just took.
 
```cpp
// On damage: start the countdown
InOutTrailingTimer = TrailingDelay; // default: 0.5s
 
// On healing or startup: snap both bars instantly
if (NewTargetPercent >= InOutTargetPercent)
{
    TrailingBar->SetPercent(NewTargetPercent);
    InOutTrailingTimer = 999.0f; // disable trailing animation
}
```
 
After the delay, the trailing bar interpolates to match using `FInterpTo` at a configurable speed. All values are `EditDefaultsOnly` — tunable from Blueprint without recompiling.
 
---
 
### 5. Data-Driven Stats via `UCharacterDataAsset`
 
No hardcoded stats in constructors. The AttributeSet constructor is explicitly empty by design:
 
```cpp
UCharacterAttributeSet::UCharacterAttributeSet()
{
    // Stats are driven by UCharacterDataAsset, not hardcoded here.
    // Violates SOLID OCP if you hardcode values here.
}
```
 
All starting values come from `FCharacterStartingStats` inside `UCharacterDataAsset`, which inherits from `UPrimaryDataAsset` for async loading via the Asset Manager.
 
---
 
## 📐 Stat Scaling Reference
 
| Primary Stat | Scales |
|:---|:---|
| **Stamina** | +10 MaxHealth per point |
| **Intellect** | +15 MaxMana, +2.5 SpellDamage per point |
| **Strength** | +2.0 AttackDamage per point |
| **Agility** | +1% Haste → `FinalCastInterval = WeaponBaseInterval / (1 + Agility * 0.01)` |
 
MaxHealth and MaxMana changes trigger proportional adjustments to current values — HP/Mana percentage is always preserved.
 
---
 
## 🔌 Integration
 
```cpp
// In HeroCharacter::BeginPlay — one call to wire everything up
UPlayerVitalsWidget* Widget = CreateWidget<UPlayerVitalsWidget>(PC, WidgetClass);
Widget->AddToViewport();
Widget->InitializeVitals(GetAbilitySystemComponent());
```
 
Widget Blueprint requirements:
- `HealthBar` + `TrailingHealthBar` — two `UProgressBar` bindings
- `ManaBar` + `TrailingManaBar` — two `UProgressBar` bindings  
- `HealthText` + `ManaText` — two `UTextBlock` bindings
 
Tunable parameters (exposed via `EditDefaultsOnly`):
- `TrailingDelay` — how long before the trailing bar starts moving (default: 0.5s)
- `MainBarInterpSpeed` — snap speed of the primary bar (default: 15.0)
- `TrailingBarInterpSpeed` — lerp speed of the delayed bar (default: 3.0)
 
---
 
## 🧩 Part of a Larger System
 
Each part of this series is one module in a broader RPG architecture. Previous and upcoming systems:
 
| # | System | Status |
|:---|:---|:---|
| 1 | Character Foundation (GAS setup, movement) | ✅ Released |
| 2 | Input & Camera System | ✅ Released |
| **3** | **HP & Mana Vitals (this repo)** | **✅ Released** |
| 4 | Combat System (abilities, damage pipeline) | 🔜 Upcoming |
| 5 | Enemy AI (Behavior Trees + GAS) | 🔜 Upcoming |
| 6 | Inventory & Equipment | 🔜 Upcoming |
| 7 | Buff/Debuff & Advanced Stat System | 🔜 Upcoming |
 
---
 
## 📌 Design Philosophy
 
- **Modularity first** — every system is portable and self-contained
- **Event-driven over polling** — update only when something actually changes
- **Data-driven over hardcoded** — designers configure, code doesn't care
- **SOLID at every layer** — orchestrators, single-responsibility helpers, DIP where it matters
 
---
 
## 📄 License
 
Provided as a reference implementation for GAS-based UI systems. Feel free to use as a foundation for your own projects.
