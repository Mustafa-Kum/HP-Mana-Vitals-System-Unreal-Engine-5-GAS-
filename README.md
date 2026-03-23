# ⚔️ AAA HP & Mana Vitals System — Unreal Engine 5 (GAS)

A production-grade, reactive HP and Mana HUD system built on top of the **Gameplay Ability System (GAS)** for Unreal Engine 5. Designed following strict **AAA game development architecture standards** — SOLID, DRY, Clean Code, and high-performance patterns used in large-scale RPG/MMO projects.

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.7-blue?logo=unrealengine)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus)
![Architecture](https://img.shields.io/badge/Architecture-AAA_Standards-gold)
![GAS](https://img.shields.io/badge/GAS-Gameplay_Ability_System-green)

---

## 🎯 Features

- **Zero-Tick Reactive UI** — Attribute changes drive UI updates via GAS delegates, not per-frame polling
- **Smooth Trailing Bars** — Dual-bar system with fast primary bar and delayed trailing bar (WoW/FFXIV-style damage trail)
- **Data-Driven Stats** — All starting stats configurable via `UCharacterDataAsset` (no hardcoded values)
- **Stat Scaling Pipeline** — Primary stats (Stamina, Intellect) automatically derive secondary stats (MaxHP, MaxMana) with proportional adjustments
- **Fully Modular** — Can be integrated into any GAS-based project with minimal dependencies
- **Designer-Friendly** — Animation speeds, trailing delays, and base stats exposed to Blueprint for easy tuning

---

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    DATA LAYER                               │
│  CharacterDataAsset ──► FCharacterStartingStats             │
│  (Data-Driven Base Stats: STR, AGI, INT, STA, Mana, etc.)  │
└─────────────────────┬───────────────────────────────────────┘
                      │ Feeds into
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                  CHARACTER LAYER                            │
│  CharacterBase::ApplyStartingStats()                        │
│  ├─ Init base vitals (HP=100, Mana=100)                    │
│  └─ Set primary stats → triggers scaling via Setters        │
│                                                             │
│  HeroCharacter::InitializeHUD()                             │
│  └─ Creates PlayerVitalsWidget → binds to ASC               │
└─────────────────────┬───────────────────────────────────────┘
                      │ ASC Delegates
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                 GAS ATTRIBUTE LAYER                         │
│  CharacterAttributeSet                                      │
│  ├─ PreAttributeChange → Orchestrator Dispatch              │
│  │   ├─ RecalculateFromStamina()   → MaxHealth scaling      │
│  │   ├─ RecalculateFromIntellect() → MaxMana + SpellDmg     │
│  │   ├─ RecalculateFromStrength()  → AttackDamage           │
│  │   ├─ RecalculateFromAgility()   → CastSpeed (Haste)      │
│  │   └─ ClampHealth() / ClampMana()                         │
│  ├─ AdjustAttributeProportionally() → DRY generic helper    │
│  └─ PostGameplayEffectExecute → Clamping after GE apply     │
└─────────────────────┬───────────────────────────────────────┘
                      │ OnAttributeValueChange Delegates
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                    UI LAYER                                  │
│  PlayerVitalsWidget (UUserWidget)                            │
│  ├─ InitializeVitals() → Binds 4 GAS delegates              │
│  ├─ RefreshVitalDisplay() → Generic Validation/Process/Exec │
│  ├─ NativeTick() → ProcessAllInterpolations()               │
│  │   ├─ ProcessMainBarInterpolation()    (Fast FInterpTo)   │
│  │   └─ ProcessTrailingBarInterpolation() (Delayed + Slow)  │
│  └─ NativeDestruct() → Clean delegate unbinding             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 File Structure

```
Source/WoWClone/
├── Public/
│   ├── Abilities/AttributeSets/
│   │   └── CharacterAttributeSet.h      # GAS attributes + scaling declarations
│   ├── Characters/
│   │   ├── CharacterBase.h              # Base class: ASC, AttributeSet, stat init
│   │   ├── HeroCharacter.h             # Player character: HUD integration
│   │   └── HeroInputComponent.h        # Enhanced Input: action bindings
│   ├── DataAssets/
│   │   └── CharacterDataAsset.h         # FCharacterStartingStats struct
│   └── UI/
│       └── PlayerVitalsWidget.h         # ★ Core widget: trailing bars, delegates
│
├── Private/
│   ├── Abilities/AttributeSets/
│   │   └── CharacterAttributeSet.cpp    # Scaling logic, proportional adjust, clamping
│   ├── Characters/
│   │   ├── CharacterBase.cpp            # Stat initialization pipeline
│   │   ├── HeroCharacter.cpp            # InitializeHUD(), TestVitals()
│   │   └── HeroInputComponent.cpp       # Input routing (DRY lambda binding)
│   └── UI/
│       └── PlayerVitalsWidget.cpp       # ★ Interpolation, orchestration, GAS callbacks
```

---

## 🔧 Key Design Decisions

### 1. Reactive UI (No Tick Polling)
```cpp
// BAD: Polling every frame (60+ unnecessary calls/sec)
void NativeTick(...) {
    float HP = ASC->GetNumericAttribute(HealthAttr);
    HealthBar->SetPercent(HP / MaxHP);
}

// GOOD: Event-driven (fires ONLY when value changes)
ASC->GetGameplayAttributeValueChangeDelegate(HealthAttr)
    .AddUObject(this, &UPlayerVitalsWidget::OnHealthChanged);
```

### 2. Orchestration Pattern (Validation → Processing → Execution)
```cpp
void RefreshVitalDisplay(const FGameplayAttribute& CurrentAttr, ...)
{
    // 1. Validation
    if (!CachedASC) return;

    // 2. Processing
    const float CurrentValue = FetchAttributeValue(CurrentAttr);
    const float MaxValue = FetchAttributeValue(MaxAttr);
    const float NewTargetPercent = CalculateTargetPercent(CurrentValue, MaxValue);

    // 3. Execution
    UpdateTrailingState(NewTargetPercent, InOutTargetPercent, ...);
    ExecuteVitalTextUpdate(ValueText, CurrentValue, MaxValue);
}
```

### 3. DRY Proportional Adjustment
```cpp
// Single generic helper replaces duplicate Stamina/Intellect scaling code
void AdjustAttributeProportionally(
    const FGameplayAttribute& CurrentAttr,
    const FGameplayAttribute& MaxAttr,
    float NewMax, float OldMax);
```

### 4. Smooth Trailing Bar System
```
Frame 0:  HealthBar ████████████████████ 100%    (Both bars full)
          TrailingBar ████████████████████ 100%

Frame 10: HealthBar ██████████░░░░░░░░░░  50%    (Main bar drops FAST)
          TrailingBar ████████████████████ 100%   (Trailing bar WAITS)

Frame 40: HealthBar ██████████░░░░░░░░░░  50%    (Main bar at target)
          TrailingBar ██████████████░░░░░░  65%   (Trailing starts moving SLOW)

Frame 80: HealthBar ██████████░░░░░░░░░░  50%    (Both settled)
          TrailingBar ██████████░░░░░░░░░░  50%
```

---

## 🎮 Stat Scaling Formula

| Primary Stat | Derived Effect | Formula |
|---|---|---|
| **Stamina** | Max Health | `1 STA = +10 MaxHP` |
| **Intellect** | Max Mana | `1 INT = +15 MaxMana` |
| **Intellect** | Spell Damage | `1 INT = +2.5 SpellDmg` |
| **Strength** | Attack Damage | `1 STR = +2.0 AtkDmg` |
| **Agility** | Cast Speed | `FinalInterval = BaseInterval / (1 + AGI * 0.01)` |

> **Proportional Adjustment:** When MaxHP increases (e.g., equipping Stamina gear), current HP scales proportionally. If you were at 80% HP before the buff, you remain at 80% after.

---

## 🔌 Integration Guide

### Prerequisites
- Unreal Engine 5.x with **Gameplay Ability System** plugin enabled
- Module dependencies: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`

### Steps
1. Add the source files to your project module
2. Create a **Widget Blueprint** (`WBP_PlayerVitals`) parented to `PlayerVitalsWidget`:
   - Add `ProgressBar` widgets named: `HealthBar`, `TrailingHealthBar`, `ManaBar`, `TrailingManaBar`
   - Add `TextBlock` widgets named: `HealthText`, `ManaText`
   - Stack trailing bars behind main bars (Z-Order: Trailing=0, Main=1, Text=2)
   - Set main bar Background Image Tint Alpha to `0.0` (transparent frame)
3. Create an **Input Action** (`IA_TestVitals`) and map it in your Input Mapping Context
4. In your Character Blueprint, assign `WBP_PlayerVitals` to `PlayerVitalsWidgetClass`
5. Play → HP/Mana bars appear with correct stat-scaled values

### Minimal Integration (Widget Only)
```cpp
// In your character's BeginPlay:
UPlayerVitalsWidget* Widget = CreateWidget<UPlayerVitalsWidget>(PC, WidgetClass);
Widget->AddToViewport();
Widget->InitializeVitals(GetAbilitySystemComponent());
// Done. The widget handles everything else reactively.
```

---

## 📐 AAA Standards Compliance

| Principle | Implementation |
|---|---|
| **SRP** | Each method does exactly one thing with an intention-revealing name |
| **OCP** | New vitals (Energy, Rage) can be added without modifying existing code |
| **DIP** | Widget depends on `UAbilitySystemComponent*` interface, not concrete classes |
| **DRY** | `RefreshVitalDisplay()` generic handler, `AdjustAttributeProportionally()` shared helper |
| **Clean Code** | `Validation → Processing → Execution` pattern in all orchestrators |
| **Performance** | Event-driven delegates, `NativeTick` only for visual interpolation |
| **Thread Safety** | All GAS delegate operations run on Game Thread (UE5 guarantee) |
| **Modularity** | Zero cross-system coupling; widget only needs an ASC pointer |

---

## 📄 License

This code is provided as an architectural showcase. Feel free to use it as reference for your own GAS-based UI systems.

---

*Built with ❤️ using Unreal Engine 5 and the Gameplay Ability System*
