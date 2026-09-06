"""Procedural SFX via pygame.mixer + numpy-ish pure Python waves."""
from __future__ import annotations

import math
import struct
import array

import pygame

_initialized = False


def init_audio() -> None:
    global _initialized
    if _initialized:
        return
    try:
        pygame.mixer.pre_init(22050, -16, 1, 512)
        pygame.mixer.init()
        _initialized = True
    except pygame.error:
        _initialized = False


def _tone(freq: float, ms: int, vol: float = 0.35, wave: str = "square") -> pygame.mixer.Sound | None:
    if not _initialized:
        return None
    rate = 22050
    n = int(rate * ms / 1000)
    buf = array.array("h")
    for i in range(n):
        t = i / rate
        env = 1.0
        # short attack/decay
        a = min(1.0, i / (rate * 0.01))
        d = max(0.0, 1.0 - (i / max(1, n - 1)) ** 1.5)
        env = a * d
        if wave == "square":
            raw = 1.0 if math.sin(2 * math.pi * freq * t) >= 0 else -1.0
        elif wave == "saw":
            raw = 2.0 * ((freq * t) % 1.0) - 1.0
        else:
            raw = math.sin(2 * math.pi * freq * t)
        sample = int(max(-1.0, min(1.0, raw * env * vol)) * 32767)
        buf.append(sample)
    try:
        return pygame.mixer.Sound(buffer=buf.tobytes())
    except Exception:
        return None


class SFX:
    def __init__(self) -> None:
        init_audio()
        self.hit = _tone(180, 60, 0.3, "square")
        self.swing = _tone(90, 40, 0.2, "saw")
        self.hurt = _tone(120, 120, 0.35, "saw")
        self.kill = _tone(320, 90, 0.3) or _tone(220, 80, 0.25)
        self.level = _tone(440, 80, 0.3)
        self.level2 = _tone(660, 120, 0.28)
        self.coin = _tone(880, 50, 0.25)
        self.shop = _tone(520, 70, 0.25)
        self.talk = _tone(300, 40, 0.18)
        self.duel = _tone(200, 200, 0.4, "saw")
        self.spell = _tone(500, 100, 0.3)
        self.heal = _tone(600, 150, 0.28)
        self.click = _tone(400, 30, 0.15)
        self.win = _tone(523, 180, 0.35)
        self.lose = _tone(110, 300, 0.4, "saw")

    def play(self, sound: pygame.mixer.Sound | None) -> None:
        if sound is not None:
            try:
                sound.play()
            except Exception:
                pass

    def level_up(self) -> None:
        self.play(self.level)
        if self.level2:
            self.level2.play()
