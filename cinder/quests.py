"""Quest definitions and state for Cinder's story through-line."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class Quest:
    id: str
    title: str
    desc: str
    hint: str
    active: bool = False
    complete: bool = False
    stage: int = 0
    # optional counters
    count: int = 0
    goal: int = 0


class QuestLog:
    def __init__(self) -> None:
        self.quests: dict[str, Quest] = {
            "prove": Quest(
                id="prove",
                title="Prove Your Worth",
                desc=(
                    "King Aldric of Ashfall Keep will not name you a free mage until you "
                    "recover the Ember Relic stolen by the Ash Drake in the Cinder Nest "
                    "to the southeast. Survive the wilds. Return with the relic."
                ),
                hint="Talk to King Aldric, then travel SE past the village.",
                active=True,
                stage=0,
            ),
            "ore": Quest(
                id="ore",
                title="Ore for the Forge",
                desc="Blacksmith Brenna needs 3 Iron Ore from bandits in the wilds.",
                hint="Defeat bandits east of the village; return ore to Brenna.",
                goal=3,
            ),
            "pelts": Quest(
                id="pelts",
                title="Pelts for the Inn",
                desc="Innkeeper Maro will pay well for 4 Wolf Pelts — and a free rest.",
                hint="Hunt wolves along the forest paths; bring pelts to Maro.",
                goal=4,
            ),
        }

    def get(self, qid: str) -> Quest:
        return self.quests[qid]

    def active_list(self) -> list[Quest]:
        return [q for q in self.quests.values() if q.active and not q.complete]

    def main_hint(self) -> str:
        for q in self.active_list():
            if q.id == "prove":
                if q.stage == 0:
                    return "Speak with King Aldric in Ashfall Keep"
                if q.stage == 1:
                    return "Find the Ash Drake SE of Ember Village"
                if q.stage == 2:
                    return "Return the Ember Relic to King Aldric"
            return q.hint
        if self.quests["prove"].complete:
            return "The Keep is safe — explore freely, grow stronger"
        return "Explore Ember Village"

    def start(self, qid: str) -> None:
        q = self.quests[qid]
        if not q.complete:
            q.active = True

    def set_stage(self, qid: str, stage: int) -> None:
        self.quests[qid].stage = stage

    def complete(self, qid: str) -> None:
        q = self.quests[qid]
        q.complete = True
        q.active = False
