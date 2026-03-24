# ⚔️ Building My RPG — Part 3: HP & Mana Vitals System (Unreal Engine 5 / GAS)

This is Part 3 of my ongoing series where I'm building a modular RPG framework in Unreal Engine 5, one gameplay system at a time.

Each part focuses on a single production-minded system that can be studied, reused, and transplanted into other projects with minimal friction.

> 🎯 **Goal:** Build a clean, modular RPG architecture around GAS, data-driven initialization, and event-driven UI.

---

## 🧠 TL;DR

This module implements a reactive HP/Mana UI system on top of GAS that:

- Updates attribute-driven UI only when values actually change
- Uses a dual-bar trailing effect for clearer damage feedback
- Preserves current HP/Mana percentages when **Stamina** or **Intellect** change max vitals
- Initializes stats from a `UCharacterDataAsset` with safe fallback defaults
- Keeps the widget decoupled from character classes by talking only to the `UAbilitySystemComponent`

Important detail: the widget still uses `NativeTick`, but only to animate bar interpolation while the UI is catching up to the latest target value. It does **not** poll attributes every frame.

---

## 🚀 What Problem This Solves

A lot of early UI implementations read HP and Mana every frame from the character and push the result straight into a progress bar. That works for prototypes, but it scales poorly and tightly couples UI to gameplay objects.

This system takes a cleaner approach:

| Problem | This module's approach |
|:---|:---|
| UI reads vitals every frame | Bind to GAS attribute change delegates |
| Widget depends on concrete character class | Widget only receives a `UAbilitySystemComponent*` |
| Max vital changes feel wrong | Current HP/Mana are adjusted proportionally when primary stat scaling changes the max |
| Damage feedback feels abrupt | Use a delayed trailing bar for visual readability |
| Missing data asset breaks setup | Fall back to safe default stats during character initialization |

---

## 🏗️ Architecture

```text
[ UCharacterDataAsset ]
        ↓
[ ACharacterBase ]               ← Initializes starting stats
        ↓
[ UCharacterAttributeSet ]       ← Owns attributes and derived-stat rules
        ↓
[ UAbilitySystemComponent ]      ← Broadcasts attribute change delegates
        ↓
[ UPlayerVitalsWidget ]          ← Reactive UI + animated bar interpolation
```

The key separation is simple:

- `ACharacterBase` handles startup stat initialization
- `UCharacterAttributeSet` owns scaling and clamping rules
- `UAbilitySystemComponent` broadcasts attribute changes
- `UPlayerVitalsWidget` renders the result without knowing anything about the concrete character class

---

## 🔑 Key Design Decisions

### 1. GAS-Driven UI Updates

The widget binds directly to GAS delegates and refreshes its target values only when relevant attributes change.

```cpp
void UPlayerVitalsWidget::BindToAbilitySystem(UAbilitySystemComponent* InASC)
{
    if (!InASC)
    {
        return;
    }

    InASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetHealthAttribute()).AddUObject(this, &UPlayerVitalsWidget::OnHealthChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UPlayerVitalsWidget::OnMaxHealthChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetManaAttribute()).AddUObject(this, &UPlayerVitalsWidget::OnManaChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetMaxManaAttribute()).AddUObject(this, &UPlayerVitalsWidget::OnMaxManaChanged);
}
```

Cleanup happens in `NativeDestruct()` so the widget does not leave stale delegate bindings behind.

```cpp
void UPlayerVitalsWidget::UnbindFromAbilitySystem()
{
    if (!CachedASC)
    {
        return;
    }

    CachedASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetHealthAttribute()).RemoveAll(this);
    CachedASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
    CachedASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetManaAttribute()).RemoveAll(this);
    CachedASC->GetGameplayAttributeValueChangeDelegate(UCharacterAttributeSet::GetMaxManaAttribute()).RemoveAll(this);
    CachedASC = nullptr;
}
```

The result is event-driven attribute updates, with tick used only for short-lived visual interpolation.

---

### 2. AttributeSet Orchestration Instead of One Giant Branch

`PreAttributeChange()` delegates work to focused helpers instead of burying every rule in one long block.

```cpp
void UCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (HandleDerivedAttributeChange(Attribute, NewValue))
    {
        return;
    }

    HandleClampedVitalChange(Attribute, NewValue);
}
```

Derived-stat rules are isolated behind named methods such as:

- `RecalculateFromStrength`
- `RecalculateFromAgility`
- `RecalculateFromWeaponInterval`
- `RecalculateFromIntellect`
- `RecalculateFromStamina`

That keeps the scaling logic readable and easy to extend.

---

### 3. Proportional Vital Preservation

When **Stamina** changes `MaxHealth`, or **Intellect** changes `MaxMana`, the current value is adjusted proportionally so the player keeps the same percentage.

```cpp
void UCharacterAttributeSet::AdjustAttributeProportionally(const FGameplayAttribute& CurrentAttr, float NewMax, float OldMax)
{
    if (OldMax <= 0.0f || NewMax == OldMax) return;

    UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
    if (!ASC)
    {
        return;
    }

    const float CurrentValue = CurrentAttr.GetNumericValue(this);
    const float NewCurrentValue = CurrentValue * (NewMax / OldMax);

    ASC->SetNumericAttributeBase(CurrentAttr, NewCurrentValue);
}
```

So if the player is at `60 / 100 HP` and a Stamina increase pushes max HP to `600`, the system keeps the same 60% ratio and moves the player to `360 / 600`.

This proportional adjustment is currently applied through the **primary-stat scaling path** in the AttributeSet.

---

### 4. Trailing Bar Feedback

Each vital uses two bars:

- A main bar that interpolates quickly toward the current target
- A trailing bar that waits briefly on damage, then catches up more slowly

That gives the player a clearer read on how much damage they just took.

```cpp
void UPlayerVitalsWidget::UpdateTrailingState(float NewTargetPercent, float& InOutTargetPercent, float& InOutTrailingTimer, UProgressBar* TrailingBar)
{
    if (NewTargetPercent >= InOutTargetPercent)
    {
        if (TrailingBar) TrailingBar->SetPercent(NewTargetPercent);
        InOutTrailingTimer = 999.0f;
    }
    else
    {
        InOutTrailingTimer = TrailingDelay;
    }

    InOutTargetPercent = NewTargetPercent;
}
```

A few implementation details:

- Healing or startup snaps the **trailing** bar immediately
- Damage starts a countdown before the trailing bar begins moving
- The widget early-outs from `NativeTick()` when no interpolation is active

This keeps the animation responsive without constantly doing unnecessary work.

---

### 5. Data-Driven Startup with Safe Fallbacks

The `UCharacterAttributeSet` constructor is intentionally empty. Initialization happens during character setup instead of hardcoding gameplay values in the AttributeSet itself.

```cpp
UCharacterAttributeSet::UCharacterAttributeSet()
{
    // Stats are initialized during character setup.
}
```

Starting values come from `FCharacterStartingStats` inside `UCharacterDataAsset`, including:

- Primary stats
- Base mana
- Crit chance
- Movement speed
- Base physical damage
- Base magic damage
- Armor

If no data asset is assigned, `ACharacterBase` applies safe fallback defaults so the character still initializes cleanly.

```cpp
if (CharacterClassData)
{
    ApplyStartingStats(CharacterClassData->StartingStats);
}
else
{
    FCharacterStartingStats FallbackStats;
    // ...fill fallback defaults...
    ApplyStartingStats(FallbackStats);
}
```

One important implementation detail: `MaxHealth` currently starts from a fixed base of `100.0f` in code, then scales upward through Stamina.

---

## 📐 Stat Scaling Reference

Current formulas in this repo:

| Primary Stat | Result |
|:---|:---|
| **Strength** | `AttackDamage += 2.0` per point |
| **Intellect** | `SpellDamage += 2.5` per point |
| **Intellect** | `MaxMana += 15.0` per point |
| **Stamina** | `MaxHealth += 10.0` per point |
| **Agility** | Recalculates final `CastSpeed` interval from weapon interval |

Agility uses this formula:

```cpp
FinalCastSpeed = WeaponBaseInterval / max(1.0 + Agility * 0.01, 0.1)
```

Notes:

- Lower final interval means faster attacks/casts
- Weapon swaps also trigger recalculation through `WeaponBaseInterval`
- Current HP/Mana percentage is preserved when Stamina/Intellect scaling changes the max

---

## 🛡️ Robustness Notes

This module also clamps vitals in two places:

- During `PreAttributeChange()`
- After gameplay effects execute in `PostGameplayEffectExecute()`

That helps prevent invalid HP/Mana values from slipping through effect-driven changes.

---

## 🔌 Integration

`AHeroCharacter` creates the widget for the local player and initializes it with the character's ASC.

```cpp
void AHeroCharacter::InitializeHUD()
{
    if (PlayerVitalsWidgetInstance || !PlayerVitalsWidgetClass || !IsLocalPlayerControlled())
    {
        return;
    }

    if (APlayerController* PC = GetPlayerController())
    {
        PlayerVitalsWidgetInstance = CreateWidget<UPlayerVitalsWidget>(PC, PlayerVitalsWidgetClass);
        if (PlayerVitalsWidgetInstance)
        {
            PlayerVitalsWidgetInstance->AddToViewport();
            PlayerVitalsWidgetInstance->InitializeVitals(GetAbilitySystemComponent());
        }
    }
}
```

### Widget Blueprint Requirements

Create a Blueprint subclass of `UPlayerVitalsWidget` and bind these exact widget names:

- `HealthBar`
- `TrailingHealthBar`
- `ManaBar`
- `TrailingManaBar`
- `HealthText`
- `ManaText`

### Exposed Tuning Parameters

These are available as `EditDefaultsOnly` on the widget:

- `TrailingDelay` — delay before the trailing bar starts moving
- `MainBarInterpSpeed` — interpolation speed of the main bar
- `TrailingBarInterpSpeed` — interpolation speed of the delayed bar

---

## ✅ Requirements

This implementation assumes:

- Unreal Engine 5
- Gameplay Ability System enabled
- A valid `UAbilitySystemComponent` on the character
- A registered `UCharacterAttributeSet`
- UMG available for the widget layer

Relevant module dependencies in this project include:

- `GameplayAbilities`
- `GameplayTasks`
- `GameplayTags`
- `UMG`
- `Slate`
- `SlateCore`

---

## 📦 Current Scope

This repo covers the vitals layer only:

- HP/Mana attributes
- Derived-stat scaling rules
- Reactive GAS-driven UI updates
- Animated trailing bars
- Data-driven startup values

It does **not** try to be a full combat framework by itself yet. Things like full combat abilities, damage pipelines, death handling, buff/debuff systems, and enemy AI belong to later modules in the series.

---

## 🧩 Part of a Larger Series

| # | System | Status |
|:---|:---|:---|
| 1 | Character Foundation (GAS setup, movement) | ✅ Released |
| 2 | Input & Camera System | ✅ Released |
| **3** | **HP & Mana Vitals (this repo)** | **✅ Released** |
| 4 | Combat System (abilities, damage pipeline) | 🔜 Upcoming |
| 5 | Enemy AI (Behavior Trees + GAS) | 🔜 Upcoming |
| 6 | Inventory & Equipment | 🔜 Upcoming |
| 7 | Buff / Debuff & Advanced Stat System | 🔜 Upcoming |

---

## 📌 Design Philosophy

- **Modularity first** — each system should be understandable and reusable in isolation
- **Event-driven where it matters** — attribute reads happen on change, not every frame
- **Data-driven startup** — designers can configure class baselines without rewriting logic
- **Focused responsibilities** — character setup, attribute logic, ASC notifications, and UI rendering each live in their own layer

---

## 📄 License

Provided as a reference implementation for GAS-based RPG UI architecture.

Feel free to study it, adapt it, and use it as a foundation for your own systems.
