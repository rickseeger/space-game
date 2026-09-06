"""Items, gear definitions, and inventory helpers."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class Item:
    id: str
    name: str
    kind: str  # weapon, armor, boots, consumable, quest, junk
    desc: str
    price: int = 0
    sell: int = 0
    dmg: int = 0
    armor: int = 0
    speed: float = 0.0
    heal: int = 0
    stackable: bool = False


CATALOG: dict[str, Item] = {
    "wooden_staff": Item("wooden_staff", "Wooden Staff", "weapon",
                         "A brittle apprentice staff. Barely hurts.", price=0, sell=2, dmg=6),
    "ashwood_staff": Item("ashwood_staff", "Ashwood Staff", "weapon",
                          "Reinforced ashwood. Hits harder.", price=45, sell=18, dmg=12),
    "ember_rod": Item("ember_rod", "Ember Rod", "weapon",
                      "Warm to the touch. Channels flame.", price=120, sell=50, dmg=20),
    "cloth_robe": Item("cloth_robe", "Cloth Robe", "armor",
                       "Thin robe. Better than nothing.", price=0, sell=1, armor=1),
    "leather_vest": Item("leather_vest", "Leather Vest", "armor",
                         "Hardened leather. Absorbs blows.", price=55, sell=22, armor=4),
    "ember_cloak": Item("ember_cloak", "Ember Cloak", "armor",
                        "Ash-woven cloak. Warm protection.", price=140, sell=55, armor=8),
    "worn_boots": Item("worn_boots", "Worn Boots", "boots",
                       "Scuffed soles. You walk.", price=0, sell=1, speed=0.0),
    "traveler_boots": Item("traveler_boots", "Traveler Boots", "boots",
                           "Light and sure-footed.", price=40, sell=15, speed=25.0),
    "boots_of_speed": Item("boots_of_speed", "Boots of Speed", "boots",
                           "Enchanted. +50 move speed.", price=160, sell=60, speed=50.0),
    "health_vial": Item("health_vial", "Health Vial", "consumable",
                        "Restores 25 HP.", price=15, sell=5, heal=25, stackable=True),
    "greater_vial": Item("greater_vial", "Greater Vial", "consumable",
                         "Restores 55 HP.", price=40, sell=12, heal=55, stackable=True),
    "iron_ore": Item("iron_ore", "Iron Ore", "junk",
                     "Raw ore. The blacksmith wants this.", price=0, sell=8, stackable=True),
    "wolf_pelt": Item("wolf_pelt", "Wolf Pelt", "junk",
                      "Rough gray fur. The innkeeper buys these.", price=0, sell=10, stackable=True),
    "ember_relic": Item("ember_relic", "Ember Relic", "quest",
                        "The Keep's stolen heartstone. Return it to the King.", price=0, sell=0),
}


@dataclass
class Inventory:
    gold: int = 20
    weapon: str = "wooden_staff"
    armor: str = "cloth_robe"
    boots: str = "worn_boots"
    bag: dict[str, int] = field(default_factory=lambda: {"health_vial": 2})

    def count(self, item_id: str) -> int:
        return self.bag.get(item_id, 0)

    def add(self, item_id: str, n: int = 1) -> None:
        self.bag[item_id] = self.bag.get(item_id, 0) + n

    def remove(self, item_id: str, n: int = 1) -> bool:
        if self.count(item_id) < n:
            return False
        self.bag[item_id] -= n
        if self.bag[item_id] <= 0:
            del self.bag[item_id]
        return True

    def weapon_item(self) -> Item:
        return CATALOG[self.weapon]

    def armor_item(self) -> Item:
        return CATALOG[self.armor]

    def boots_item(self) -> Item:
        return CATALOG[self.boots]

    def equip(self, item_id: str) -> Optional[str]:
        item = CATALOG.get(item_id)
        if not item or self.count(item_id) < 1:
            return None
        slot = item.kind
        if slot not in ("weapon", "armor", "boots"):
            return None
        old = getattr(self, slot)
        self.remove(item_id, 1)
        self.add(old, 1)
        setattr(self, slot, item_id)
        return old
