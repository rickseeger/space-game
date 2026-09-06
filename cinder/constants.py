"""Shared constants, palette, and tuning for Cinder."""

TITLE = "Cinder"
VERSION = "1.0.0"

# Display
WIDTH, HEIGHT = 1280, 720
FPS = 60
TILE_W, TILE_H = 64, 32  # isometric diamond
CHAR_SCALE = 1.0

# World
MAP_W, MAP_H = 80, 80
CASTLE_CX, CASTLE_CY = 18, 18
VILLAGE_CX, VILLAGE_CY = 28, 22
BOSS_CX, BOSS_CY = 62, 55

# Colors — vibrant classic RPG (DQ/FF energy), wizard keeps warm ember accents
BG = (72, 168, 232)           # bright sky blue (void / off-map)
SKY = (110, 190, 245)
ASH = (186, 120, 168)         # lilac ash clearing (not muddy gray)
EMBER = (255, 110, 40)        # saturated ember orange
EMBER_HOT = (255, 200, 60)    # bright gold-orange
MAGENTA = (255, 70, 180)
CYAN = (60, 230, 255)
GOLD = (255, 214, 70)
CREAM = (255, 252, 240)
INK = (35, 40, 70)            # deep blue-ink, not brown
STONE = (170, 178, 198)       # cool light stone
STONE_LT = (210, 218, 235)
GRASS = (86, 196, 78)         # saturated spring green
GRASS_DK = (48, 158, 62)
FOREST = (34, 140, 58)        # rich forest green
PATH = (232, 198, 110)        # sunny sand path
PATH_DK = (210, 168, 78)
WATER = (56, 170, 255)        # bright turquoise-blue
WATER_DK = (30, 120, 220)
CASTLE = (200, 205, 225)      # pale lavender-stone walls
CASTLE_DK = (140, 148, 180)
ROOF = (230, 55, 55)          # vivid red roofs
ROOF_TEAL = (30, 180, 170)
ROOF_YELLOW = (255, 200, 40)
WOOD = (180, 110, 50)
HP_RED = (255, 70, 90)
HP_BG = (80, 30, 50)
XP_BLUE = (70, 160, 255)
UI_PANEL = (40, 55, 110)      # royal blue panels
UI_PANEL2 = (55, 40, 100)
UI_BORDER = (255, 210, 70)    # bright gold borders
DUEL_GLOW = (255, 80, 255)    # neon magenta
DUEL_GLOW2 = (80, 255, 255)   # neon cyan
SHADOW = (20, 40, 80, 70)

# Player base stats (weak start)
PLAYER_MAX_HP = 45
PLAYER_SPEED = 110.0
PLAYER_MELEE_DMG = 6
PLAYER_MELEE_RANGE = 42
PLAYER_MELEE_CD = 0.38
PLAYER_XP_BASE = 20

# Death
GOLD_LOSS_FRAC = 0.25

# Keys helper labels
CONTROLS_HELP = [
    "WASD / Arrows — Move",
    "Space / LMB — Attack / Cast",
    "E / RMB — Interact / Talk",
    "I — Inventory",
    "Q — Quest log",
    "1 — Staff melee  2 — Emberbolt (when learned)",
    "Esc — Pause",
]
