// this was vibe coded xd,
// don't take this seriosly,
// but feel free to use this.
// It turned out to be actually pretty nice,
// but someone may mind this.
//
// Peace :) - danihek

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
/*
 * sysinfo can only be used on linux and in this code here uses it only at 
 * one location. sysconf is actually from <unistd.h> which is 
 * already included
 * */
#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#include <ctype.h>
#include <stdarg.h>

const char *postProcessingFs =
    "#version 330 core\n"
    "in vec2 fragTexCoord;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0; uniform sampler2D bloomTexture; uniform sampler2D bloomTextureHiRes;\n"
    "uniform float time; uniform vec2 resolution;\n"
    "uniform float glitchIntensity; uniform float blurIntensity;\n"
    "uniform float pixelSize; uniform int mirrorMode;\n"
    "uniform float shakeIntensity; uniform float collapseIntensity;\n"
    "uniform float scanlineIntensity;\n"
    "const float bloomFactor = 1.0; const float vignetteStrength = 0.3;\n"
    "const float grainStrength = 0.04;\n"
    "float random(vec2 st) { return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123); }\n"
    "vec2 barrelDistortion(vec2 uv, float strength) { vec2 p=uv*2.0-1.0; float r2=dot(p,p); p*=(1.0+strength*r2); return p*0.5+0.5; }\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    if (shakeIntensity > 0.0) { uv += vec2(random(vec2(time*10.0))-0.5, random(vec2(time*10.0+0.5))-0.5) * shakeIntensity * 0.1; }\n"
    "    if (pixelSize > 1.0) { uv = floor(uv * pixelSize) / pixelSize; }\n"
    "    if (mirrorMode == 1) uv.x = 1.0 - uv.x;\n"
    "    if (collapseIntensity > 0.0) { uv = barrelDistortion(uv, collapseIntensity*2.0); if(length(uv*2.0-1.0) > 1.0-collapseIntensity) {discard;} }\n"
    "    vec3 baseColor = vec3(0.0);\n"
    "    if (blurIntensity > 0.0) { \n"
    "        vec2 center = vec2(0.5); \n"
    "        baseColor += texture(texture0, uv).rgb; \n"
    "        for (int i = 1; i < 10; i++) { \n"
    "           vec2 offset = normalize(uv - center) * blurIntensity * float(i)/100.0; \n"
    "           baseColor += texture(texture0, uv + offset).rgb; \n"
    "           baseColor += texture(texture0, uv - offset).rgb; \n"
    "        } baseColor /= 21.0; \n"
    "    } else { baseColor = texture(texture0, uv).rgb; }\n"
    "    if (glitchIntensity > 0.0) { \n"
    "       float lineJitter = random(vec2(uv.y * 10.0, time)) * 0.03 * glitchIntensity; \n"
    "       float blocky = random(vec2(floor(uv.y*10.0)/10.0, time)) > 0.95 ? 0.1 : 0.0; \n"
    "       baseColor.r = texture(texture0, uv + vec2(lineJitter + blocky, 0.0)).r; \n"
    "       baseColor.g = texture(texture0, uv).g; \n"
    "       baseColor.b = texture(texture0, uv - vec2(lineJitter, 0.0)).b; \n"
    "    }\n"
    "    vec4 bloomColor = texture(bloomTexture, uv);\n"
    "    vec4 bloomColorHiRes = texture(bloomTextureHiRes, uv);\n"
    "    vec3 combinedColor = baseColor + (bloomColor.rgb + bloomColorHiRes.rgb) * bloomFactor;\n"
    "    float vignette = 1.0 - vignetteStrength * pow(length(fragTexCoord * 2.0 - 1.0), 2.0);\n"
    "    combinedColor *= vignette - collapseIntensity;\n"
    "    if (scanlineIntensity > 0.0) { combinedColor *= (1.0 - (sin(fragTexCoord.y * 400.0) * 0.5 + 0.5) * scanlineIntensity * 0.1); }\n"
    "    float grain = (random(fragTexCoord + time) - 0.5) * grainStrength;\n"
    "    combinedColor += grain;\n"
    "    finalColor = vec4(combinedColor, 1.0);\n"
    "}";
const char *blurFs =
    "#version 330 core\n"
    "in vec2 fragTexCoord;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec2 renderSize;\n"
    "uniform bool horizontal;\n"
    "const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n"
    "void main() {\n"
    "    vec2 texel = 1.0/renderSize;\n"
    "    vec3 result = texture(texture0, fragTexCoord).rgb * weight[0];\n"
    "    if(horizontal) {\n"
    "        for(int i=1; i<5; i++) {\n"
    "            result += texture(texture0, fragTexCoord + vec2(texel.x*i, 0.0)).rgb * weight[i];\n"
    "            result += texture(texture0, fragTexCoord - vec2(texel.x*i, 0.0)).rgb * weight[i];\n"
    "        }\n"
    "    } else {\n"
    "        for(int i=1; i<5; i++) {\n"
    "            result += texture(texture0, fragTexCoord + vec2(0.0, texel.y*i)).rgb * weight[i];\n"
    "            result += texture(texture0, fragTexCoord - vec2(0.0, texel.y*i)).rgb * weight[i];\n"
    "        }\n"
    "    }\n"
    "    finalColor = vec4(result, 1.0);\n"
    "}";

typedef struct
{
    Color bg;
    Color idle;
    Color hover;
    Color border;
    Color ripple;
    Color overlay;
    Color text;
} Theme;

Theme AppTheme;

#define MAX_POSSIBLE_WALLPAPERS 2048
#define MAX_POSSIBLE_PARTICLES 256
#define MAX_TEXTURES_TO_LOAD_PER_FRAME 4

int g_base_thumb_size;
int g_max_wallpapers;
int g_particle_count;
int g_base_padding;
int g_max_threads;
int g_max_fps;
float g_anim_speed;
float g_ken_burns_duration;
float g_border_thickness_bloom;

typedef enum {
    MODE_GRID = 0,
    MODE_RIVER_H,
    MODE_RIVER_V,
    MODE_WAVE
} LayoutMode;
#define NUM_MODES 4

typedef struct {
    float hoverAnim;
    atomic_bool loaded;
    char *path, *filename;

    Texture2D texture;
    Vector2 animPos, animSize;
} Wallpaper;

typedef struct
{
    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
} Particle;

typedef enum
{
    EFFECT_NONE,
    EFFECT_GLITCH,
    EFFECT_BLUR,
    EFFECT_PIXELATE,
    EFFECT_REVEAL,
    EFFECT_SHAKE,
    EFFECT_COLLAPSE
} EffectType;

static Wallpaper wallpapers[MAX_POSSIBLE_WALLPAPERS];
static Particle particles[MAX_POSSIBLE_PARTICLES];

static int wallpaper_count = 0;
static bool print_filename_only = false;

static atomic_int next_load_index = 0;
static atomic_bool loader_running = true;

static Image pendingImages[MAX_POSSIBLE_WALLPAPERS];
static atomic_bool imagePending[MAX_POSSIBLE_WALLPAPERS];

static int preview_index = -1;
static float previewAnim = 0.0f;
static int closing_preview_index = -1;

static Texture2D fullPreviewTexture;
static atomic_bool isFullTextureReady = false;
static Image pendingFullImage;
static atomic_bool fullImagePending = false;

static bool isExiting = false;
static float masterScale = 1.0f;

static char searchBuffer[256] = {0};
static int searchBufferCount = 0;
static bool isSearching = false;

static float kenBurnsTimer = 0.0f;

static EffectType g_startupEffect = EFFECT_NONE;
static EffectType g_keypressEffect = EFFECT_NONE;
static EffectType g_exitEffect = EFFECT_NONE;

static float g_effectIntensity = 0.0f;
static float g_effectTimer = 0.0f;
static float g_effectDuration = 0.0f;
static EffectType g_activeEffect = EFFECT_NONE;

static LayoutMode g_currentMode = MODE_GRID;
static LayoutMode g_targetMode = MODE_GRID;
static float g_modeTransitionTimer = 0.0f;
static const float g_modeTransitionDuration = 1.0f;

static Vector2 g_scroll = {0, 0};
static float g_smoothScrollY = 0.0f;
static float g_smoothScrollX = 0.0f;

static int g_hoveredIndex = -1;
static Rectangle g_previewStartRect;

int g_win_width;
int g_win_height;

void LogMessage(int level, const char *format, ...)
{
    const char* level_str = "";
    const char* color_code = "";
    const char* color_reset = "\033[0m";

    switch (level) {
        case LOG_INFO:
            level_str = "INFO";
            color_code = "\033[0;34m";
            break;
        case LOG_WARNING:
            level_str = "WARN";
            color_code = "\033[0;33m";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            color_code = "\033[0;31m";
            break;
    }

    fprintf(stderr, "%s[%s] ", color_code, level_str);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "%s\n", color_reset);
}

static const char *get_home_dir()
{
    const char *home = getenv("HOME");
    if (!home)
    {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
        {
            home = pw->pw_dir;
        }
    }
    return home ? home : ".";
}

const char* stristr(const char* haystack, const char* needle)
{
    if (!needle || !*needle)
    {
        return haystack;
    }
    for (; *haystack; ++haystack)
    {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && (tolower((unsigned char)*h) == tolower((unsigned char)*n)))
        {
            h++;
            n++;
        }
        if (!*n)
        {
            return haystack;
        }
    }
    return NULL;
}

void TriggerParticleBurst(Vector2 pos)
{
    for (int i = 0; i < g_particle_count; i++)
    {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float speed = (float)GetRandomValue(200, 400);
        particles[i] = (Particle){
            .pos = pos,
            .vel = {sinf(angle) * speed, cosf(angle) * speed},
            .life = 1.0f,
            .color = Fade(AppTheme.ripple, 0.5f + (float)GetRandomValue(0, 50) / 100.0f)
        };
    }
}

static Image LoadThumbnailImage(const char *fp)
{
    Image i = LoadImage(fp);
    if (i.data)
    {
        int cs = (i.width < i.height) ? i.width : i.height;
        Rectangle cr = {
            (float)(i.width - cs) / 2,
            (float)(i.height - cs) / 2,
            (float)cs,
            (float)cs
        };
        ImageCrop(&i, cr);
        ImageResize(&i, g_base_thumb_size, g_base_thumb_size);
    }
    else
    {
        LogMessage(LOG_WARNING, "Failed to load thumbnail for: %s", fp);
    }
    return i;
}

void *FullPreviewLoaderThread(void* p)
{
    Image i = LoadImage((const char*)p);
    if (i.data)
    {
        pendingFullImage = i;
        atomic_store(&fullImagePending, true);
    }
    return NULL;
}

void *LoaderThread(void* a)
{
    while (atomic_load(&loader_running))
    {
        int i = atomic_fetch_add(&next_load_index, 1);
        if (i >= wallpaper_count)
        {
            usleep(100000);
            continue;
        }
        if (!atomic_load(&imagePending[i]) && !atomic_load(&wallpapers[i].loaded))
        {
            Image img = LoadThumbnailImage(wallpapers[i].path);
            if (img.data)
            {
                pendingImages[i] = img;
                atomic_store(&imagePending[i], true);
            }
        }
    }
    return NULL;
}

void TriggerEffect(EffectType type, float duration)
{
    if (type == EFFECT_NONE)
    {
        return;
    }
    g_activeEffect = type;
    g_effectTimer = duration;
    g_effectDuration = duration;
}

EffectType ParseEffect(const char* arg)
{
    if (strcasecmp(arg, "none") == 0) return EFFECT_NONE;
    if (strcasecmp(arg, "glitch") == 0) return EFFECT_GLITCH;
    if (strcasecmp(arg, "blur") == 0) return EFFECT_BLUR;
    if (strcasecmp(arg, "pixelate") == 0) return EFFECT_PIXELATE;
    if (strcasecmp(arg, "reveal") == 0) return EFFECT_REVEAL;
    if (strcasecmp(arg, "shake") == 0) return EFFECT_SHAKE;
    if (strcasecmp(arg, "collapse") == 0) return EFFECT_COLLAPSE;

    LogMessage(LOG_WARNING, "Unrecognized effect '%s'. Defaulting to 'none'.", arg);
    return EFFECT_NONE;
}

static void LoadWallpapers(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp)
    {
        LogMessage(LOG_ERROR, "Could not open wallpaper directory: %s", dir);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL && wallpaper_count < g_max_wallpapers)
    {
        if (entry->d_type != DT_REG)
        {
            continue;
        }
        const char *ext = strrchr(entry->d_name, '.');
        if (!ext || (strcasecmp(ext, ".jpg") != 0 && strcasecmp(ext, ".jpeg") != 0 && strcasecmp(ext, ".png") != 0))
        {
            continue;
        }

        char *fullpath;
        if (asprintf(&fullpath, "%s/%s", dir, entry->d_name) == -1) continue;

        wallpapers[wallpaper_count] = (Wallpaper){
            .path = strdup(fullpath),
            .filename = strdup(entry->d_name),
            .loaded = false,
            .hoverAnim = 0.0f,
            .animPos = {(float)GetRandomValue(-500, 500), (float)GetRandomValue(800, 1200)},
            .animSize = {g_base_thumb_size, g_base_thumb_size}
        };
        atomic_store(&imagePending[wallpaper_count], false);
        wallpaper_count++;
    }
    closedir(dp);
}

void LoadDefaultConfig()
{
    AppTheme.bg = (Color){10, 10, 15, 255};
    AppTheme.idle = (Color){30, 30, 46, 255};
    AppTheme.hover = (Color){49, 50, 68, 255};
    AppTheme.border = (Color){203, 166, 247, 255};
    AppTheme.ripple = (Color){245, 194, 231, 255};
    AppTheme.overlay = (Color){10, 10, 15, 200};
    AppTheme.text = (Color){202, 212, 241, 255};

    g_startupEffect = EFFECT_NONE;
    g_keypressEffect = EFFECT_NONE;
    g_exitEffect = EFFECT_NONE;

    g_max_wallpapers = 512;
    g_base_thumb_size = 150;
    g_base_padding = 15;
    g_border_thickness_bloom = 3.0f;
    
    g_max_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
    g_anim_speed = 30.0f;
    g_particle_count = 50;
    g_ken_burns_duration = 15.0f;
    g_max_fps = 200;

    g_win_width = 1280;
    g_win_height = 720;
}

char* trim_whitespace(char* str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void ParseConfigFile()
{
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/.config/hellpaper", get_home_dir());
    mkdir(config_path, 0755);
    snprintf(config_path, sizeof(config_path), "%s/.config/hellpaper/hellpaper.conf", get_home_dir());

    FILE *file = fopen(config_path, "r");
    if (!file) {
        LogMessage(LOG_INFO, "Config file not found at '%s'. Using default settings.", config_path);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n') continue;

        char* key = strtok(line, "=");
        char* value = strtok(NULL, "\n");

        if (key && value) {
            key = trim_whitespace(key);
            value = trim_whitespace(value);

            if (strcmp(key, "bg") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.bg.r, &AppTheme.bg.g, &AppTheme.bg.b, &AppTheme.bg.a);
            else if (strcmp(key, "width") == 0) g_win_width = atoi(value);
            else if (strcmp(key, "height") == 0) g_win_height = atoi(value);
            else if (strcmp(key, "idle") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.idle.r, &AppTheme.idle.g, &AppTheme.idle.b, &AppTheme.idle.a);
            else if (strcmp(key, "hover") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.hover.r, &AppTheme.hover.g, &AppTheme.hover.b, &AppTheme.hover.a);
            else if (strcmp(key, "border") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.border.r, &AppTheme.border.g, &AppTheme.border.b, &AppTheme.border.a);
            else if (strcmp(key, "ripple") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.ripple.r, &AppTheme.ripple.g, &AppTheme.ripple.b, &AppTheme.ripple.a);
            else if (strcmp(key, "overlay") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.overlay.r, &AppTheme.overlay.g, &AppTheme.overlay.b, &AppTheme.overlay.a);
            else if (strcmp(key, "text") == 0) sscanf(value, "%hhu, %hhu, %hhu, %hhu", &AppTheme.text.r, &AppTheme.text.g, &AppTheme.text.b, &AppTheme.text.a);
            
            else if (strcmp(key, "max_wallpapers") == 0) g_max_wallpapers = atoi(value);
            else if (strcmp(key, "base_thumb_size") == 0) g_base_thumb_size = atoi(value);
            else if (strcmp(key, "base_padding") == 0) g_base_padding = atoi(value);
            else if (strcmp(key, "border_thickness_bloom") == 0) g_border_thickness_bloom = atof(value);
            else if (strcmp(key, "max_threads") == 0) g_max_threads = atoi(value);
            else if (strcmp(key, "anim_speed") == 0) g_anim_speed = atof(value);
            else if (strcmp(key, "particle_count") == 0) g_particle_count = atoi(value);
            else if (strcmp(key, "ken_burns_duration") == 0) g_ken_burns_duration = atof(value);
            else if (strcmp(key, "max_fps") == 0) g_max_fps = atoi(value);

            else if (strcmp(key, "startup_effect") == 0) g_startupEffect = ParseEffect(value);
            else if (strcmp(key, "keypress_effect") == 0) g_keypressEffect = ParseEffect(value);
            else if (strcmp(key, "exit_effect") == 0) g_exitEffect = ParseEffect(value);
        }
    }
    fclose(file);
}

void print_help()
{
    printf("Hellpaper - wallpaper picker for Linux.\n\n");
    printf("USAGE:\n");
    printf("  hellpaper [OPTIONS] [PATH]\n\n");
    printf("ARGUMENTS:\n");
    printf("  [PATH]              Optional path to the directory containing wallpapers.\n");
    printf("                      Defaults to '~/Pictures/'.\n\n");
    printf("OPTIONS:\n");
    printf("  --help              Show this help message and exit.\n");
    printf("  --filename          Print only the filename of the selected wallpaper to stdout.\n");
    printf("  --width <pixels>    Set the initial window width.\n");
    printf("  --height <pixels>   Set the initial window height.\n");
    printf("  --startup-effect <effect>\n");
    printf("  --keypress-effect <effect>\n");
    printf("  --exit-effect <effect>\n");
    printf("                      Override the configured visual effects on certain events.\n");
    printf("                      Available effects: none, glitch, blur, pixelate, shake, collapse, reveal\n\n");
    printf("KEYBINDINGS:\n");
    printf("  NAVIGATION:\n");
    printf("    h, j, k, l / Arrows Move selection. Keys repeat when held.\n");
    printf("    Mouse Wheel       Scroll through wallpapers.\n");
    printf("    Ctrl + Mouse Wheel  Zoom thumbnail scaling.\n\n");
    printf("  ACTIONS:\n");
    printf("    Enter / LMB         Select the highlighted wallpaper and exit.\n");
    printf("    L-Shift / RMB       Show a full-screen preview of the highlighted wallpaper.\n");
    printf("    /                   Enter search mode. Type to filter wallpapers by name.\n");
    printf("    ESC                 Closes Preview, then Search, then the App.\n\n");
    printf("  VIEW MODES:\n");
    printf("    1, 2, 3, 4          Switch between different layout modes:\n");
    printf("                        1: Grid\n");
    printf("                        2: Horizontal River\n");
    printf("                        3: Vertical River\n");
    printf("                        4: Wave\n\n");
    printf("CONFIGURATION:\n");
    printf("  Hellpaper can be fully customized by editing the configuration file located at:\n");
    printf("  ~/.config/hellpaper/hellpaper.conf\n");
}

void UpdateAndDrawScene(int filteredCount, int* filteredIndices, float delta, bool isPreviewing, bool isSearching)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    if (g_modeTransitionTimer > 0)
    {
        g_modeTransitionTimer -= delta;
        if (g_modeTransitionTimer <= 0)
        {
            g_currentMode = g_targetMode;
        }
    }

    for (int j = 0; j < filteredCount; j++)
    {
        int i = filteredIndices[j];
        Vector2 targetPos2D = {0}, targetSize2D = {0};
        switch (g_targetMode)
        {
            case MODE_GRID:
            {
                float ts = g_base_thumb_size * masterScale, p = g_base_padding * masterScale;
                int c = sw / (ts + p);
                if (c < 1) c = 1;
                targetPos2D = (Vector2){(j % c) * (ts + p) + p, (j / c) * (ts + p) + p - g_smoothScrollY};
                targetSize2D = (Vector2){ts, ts};
                break;
            }
            case MODE_RIVER_H:
            {
                float x = j * (g_base_thumb_size * 0.7f * masterScale) - g_smoothScrollX;
                float d = fabs(x - (sw / 2.f - g_base_thumb_size / 2.f));
                float s = Remap(d, 0, sw / 2.f, 1.5f, 0.5f);
                s = Clamp(s, 0.5, 1.5) * masterScale;
                targetPos2D = (Vector2){x, sh / 2.f - (g_base_thumb_size * s) / 2.f};
                targetSize2D = (Vector2){g_base_thumb_size * s, g_base_thumb_size * s};
                break;
            }
            case MODE_RIVER_V:
            {
                float y = j * (g_base_thumb_size * 0.7f * masterScale) - g_smoothScrollY;
                float d = fabs(y - (sh / 2.f - g_base_thumb_size / 2.f));
                float s = Remap(d, 0, sh / 2.f, 1.5f, 0.5f);
                s = Clamp(s, 0.5, 1.5) * masterScale;
                targetPos2D = (Vector2){sw / 2.f - (g_base_thumb_size * s) / 2.f, y};
                targetSize2D = (Vector2){g_base_thumb_size * s, g_base_thumb_size * s};
                break;
            }
            case MODE_WAVE:
            {
                float x = j * (g_base_thumb_size * 0.8f * masterScale) - g_smoothScrollX;
                float yOff = sinf(j * 0.2f + GetTime() * 2.f) * 80.f * masterScale;
                targetPos2D = (Vector2){x, sh / 2.f + yOff - g_base_thumb_size / 2.f};
                targetSize2D = (Vector2){g_base_thumb_size * masterScale, g_base_thumb_size * masterScale};
                break;
            }
        }
        wallpapers[i].animPos = Vector2Lerp(wallpapers[i].animPos, targetPos2D, delta * g_anim_speed);
        wallpapers[i].animSize = Vector2Lerp(wallpapers[i].animSize, targetSize2D, delta * g_anim_speed);
    }

    if (Vector2LengthSqr(GetMouseDelta()) > 0.1f) 
    {
        g_hoveredIndex = -1; 
        for (int j = filteredCount - 1; j >= 0; j--)
        {
            int i = filteredIndices[j];
            Rectangle rect = {wallpapers[i].animPos.x, wallpapers[i].animPos.y, wallpapers[i].animSize.x, wallpapers[i].animSize.y};
            if (CheckCollisionPointRec(mouse, rect) && !isPreviewing && !isSearching)
            {
                g_hoveredIndex = i;
                break;
            }
        }
    }

    int highlighted_j = -1;
    for (int j = 0; j < filteredCount; j++)
    {
        int i = filteredIndices[j];
        wallpapers[i].hoverAnim = Lerp(wallpapers[i].hoverAnim, (g_hoveredIndex == i) ? 1.f : 0.f, delta * g_anim_speed * 2.0f);
        if (g_hoveredIndex == i)
        {
            highlighted_j = j;
        }
    }

    for (int j = 0; j < filteredCount; j++)
    {
        if (j == highlighted_j) continue;
        int i = filteredIndices[j];
        Rectangle rect = {wallpapers[i].animPos.x, wallpapers[i].animPos.y, wallpapers[i].animSize.x, wallpapers[i].animSize.y};
        DrawRectangleRounded(rect, 0.1f, 8, ColorLerp(AppTheme.idle, AppTheme.hover, wallpapers[i].hoverAnim));
        if (atomic_load(&wallpapers[i].loaded))
        {
            DrawTexturePro(wallpapers[i].texture, (Rectangle){0, 0, g_base_thumb_size, g_base_thumb_size}, rect, (Vector2){0, 0}, 0, WHITE);
        }
        else
        {
            DrawCircleSectorLines(Vector2Add((Vector2){rect.x, rect.y}, (Vector2){rect.width / 2, rect.height / 2}), rect.width / 3, fmodf(GetTime() * 360, 360), fmodf(GetTime() * 360, 360) + 90, 30, Fade(AppTheme.border, 0.6f));
        }
    }

    if (highlighted_j != -1)
    {
        int i = filteredIndices[highlighted_j];
        Rectangle rect = {wallpapers[i].animPos.x, wallpapers[i].animPos.y, wallpapers[i].animSize.x, wallpapers[i].animSize.y};
        if (!isPreviewing)
            DrawRectangleRounded(rect, 0.1f, 8, ColorLerp(AppTheme.idle, AppTheme.hover, wallpapers[i].hoverAnim));
        if (atomic_load(&wallpapers[i].loaded))
        {
            DrawTexturePro(wallpapers[i].texture, (Rectangle){0, 0, g_base_thumb_size, g_base_thumb_size}, rect, (Vector2){0, 0}, 0, WHITE);
        }
        else
        {
            DrawCircleSectorLines(Vector2Add((Vector2){rect.x, rect.y}, (Vector2){rect.width / 2, rect.height / 2}), rect.width / 3, fmodf(GetTime() * 360, 360), fmodf(GetTime() * 360, 360) + 90, 30, Fade(AppTheme.border, 0.6f));
        }
        const char *text = wallpapers[i].filename;
        float fontSize = 14.f * Clamp(masterScale, 0.8f, 1.5f);
        int textWidth = MeasureText(text, fontSize);
        DrawText(text, rect.x + (rect.width - textWidth) / 2, rect.y + rect.height + 5, fontSize, AppTheme.text);
    }

    if (previewAnim > 0.001f)
    {
        DrawRectangle(0, 0, sw, sh, Fade(AppTheme.overlay, previewAnim * 0.9f));

        int current_preview_idx = (preview_index != -1) ? preview_index : closing_preview_index;
        if (current_preview_idx == -1)
        {
            return;
        }

        Texture2D* tex = (atomic_load(&isFullTextureReady) && preview_index != -1) ? &fullPreviewTexture : &wallpapers[current_preview_idx].texture;
        Rectangle texRect = {0, 0, (float)tex->width, (float)tex->height};
        float aspect = (float)texRect.width / (float)texRect.height;
        float tH = sh * 0.9f, tW = tH * aspect;
        if (tW > sw * 0.9f)
        {
            tW = sw * 0.9f;
            tH = tW / aspect;
        }

        Rectangle endRect = {(sw - tW) / 2, (sh - tH) / 2, tW, tH};
        Rectangle startRect = g_previewStartRect;

        if (preview_index == -1)
        {
            startRect = (Rectangle){wallpapers[closing_preview_index].animPos.x, wallpapers[closing_preview_index].animPos.y, wallpapers[closing_preview_index].animSize.x, wallpapers[closing_preview_index].animSize.y};
        }

        Rectangle cur = {
            Lerp(startRect.x, endRect.x, previewAnim),
            Lerp(startRect.y, endRect.y, previewAnim),
            Lerp(startRect.width, endRect.width, previewAnim),
            Lerp(startRect.height, endRect.height, previewAnim)
        };

        DrawTexturePro(*tex, texRect, cur, (Vector2){0,0}, 0.f, WHITE);
    }
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0)
        {
            print_help();
            return 0;
        }
    }

    char *wallpaper_path = NULL;
    char default_path[1024];
    LoadDefaultConfig();
    ParseConfigFile();
    SetTraceLogLevel(LOG_ERROR);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--startup-effect") == 0 && i + 1 < argc)
        {
            g_startupEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--keypress-effect") == 0 && i + 1 < argc)
        {
            g_keypressEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--exit-effect") == 0 && i + 1 < argc)
        {
            g_exitEffect = ParseEffect(argv[++i]);
        }
        else if (strcmp(argv[i], "--filename") == 0)
        {
            print_filename_only = true;
        }
        else if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            g_win_width = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            g_win_height = atoi(argv[++i]);
        }
        else if (argv[i][0] != '-' && wallpaper_path == NULL)
        {
            wallpaper_path = argv[i];
        }
    }

    if (!wallpaper_path)
    {
        snprintf(default_path, sizeof(default_path), "%s/Pictures", get_home_dir());
        wallpaper_path = default_path;
    }
    LoadWallpapers(wallpaper_path);

    InitWindow(g_win_width, g_win_height, "Hellpaper");
    SetExitKey(KEY_NULL);
    SetTargetFPS(g_max_fps);

    const float bloomDownscale = 4.0f;
    Shader postShader = LoadShaderFromMemory(NULL, postProcessingFs);
    int timeLoc = GetShaderLocation(postShader, "time");
    int resLoc = GetShaderLocation(postShader, "resolution");
    int glitchLoc = GetShaderLocation(postShader, "glitchIntensity");
    int blurLoc = GetShaderLocation(postShader, "blurIntensity");
    int pixelLoc = GetShaderLocation(postShader, "pixelSize");
    int mirrorLoc = GetShaderLocation(postShader, "mirrorMode");
    int shakeLoc = GetShaderLocation(postShader, "shakeIntensity");
    int collapseLoc = GetShaderLocation(postShader, "collapseIntensity");
    int scanlineLoc = GetShaderLocation(postShader, "scanlineIntensity");

    Shader blurShader = LoadShaderFromMemory(NULL, blurFs);
    RenderTexture2D mainTarget = LoadRenderTexture(g_win_width, g_win_height);
    RenderTexture2D bloomMask = LoadRenderTexture(g_win_width / bloomDownscale, g_win_height / bloomDownscale);
    RenderTexture2D blurPingPong = LoadRenderTexture(g_win_width / bloomDownscale, g_win_height / bloomDownscale);
    RenderTexture2D bloomMaskHiRes = LoadRenderTexture(g_win_width, g_win_height);

    pthread_t loader_threads[g_max_threads];
    for (int t = 0; t < g_max_threads; t++)
    {
        pthread_create(&loader_threads[t], NULL, LoaderThread, NULL);
    }
    TriggerEffect(g_startupEffect, 1.0f);
    
    float keyRepeatTimer = 0.0f;
    const float KEY_REPEAT_DELAY = 0.1f;

    while (!WindowShouldClose())
    {
        float delta = GetFrameTime();
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        if (isExiting && g_effectTimer <= 0)
        {
            break;
        }

        if (IsWindowResized())
        {
            UnloadRenderTexture(mainTarget);
            UnloadRenderTexture(bloomMask);
            UnloadRenderTexture(blurPingPong);
            UnloadRenderTexture(bloomMaskHiRes);
            mainTarget = LoadRenderTexture(sw, sh);
            bloomMask = LoadRenderTexture(sw / bloomDownscale, sh / bloomDownscale);
            blurPingPong = LoadRenderTexture(sw / bloomDownscale, sh / bloomDownscale);
            bloomMaskHiRes = LoadRenderTexture(sw, sh);
        }

        if (g_effectTimer > 0)
        {
            g_effectTimer -= delta;
            float p = 1.f - (g_effectTimer / g_effectDuration);
            if (g_activeEffect == EFFECT_COLLAPSE || g_activeEffect == EFFECT_REVEAL)
            {
                g_effectIntensity = p;
            }
            else
            {
                g_effectIntensity = sinf(p * PI);
            }
        }
        else
        {
            g_effectIntensity = 0.f;
            g_activeEffect = EFFECT_NONE;
        }

        bool isPreviewing = (preview_index != -1);

        int filteredIndices[g_max_wallpapers];
        int filteredCount = 0;
        for (int i = 0; i < wallpaper_count; i++)
        {
            if (searchBufferCount == 0 || stristr(wallpapers[i].filename, searchBuffer) != NULL)
            {
                filteredIndices[filteredCount++] = i;
            }
        }

        float maxScrollY = 0, maxScrollX = 0;
        switch(g_currentMode)
        {
            case MODE_GRID:
                {
                    float ts = g_base_thumb_size * masterScale, p = g_base_padding * masterScale;
                    int c = sw / (ts + p); if (c < 1) c = 1;
                    int r = (filteredCount + c - 1) / c;
                    if (r > 0) maxScrollY = r * (ts + p) - sh + p;
                    break;
                }
            case MODE_RIVER_V:
                {
                    maxScrollY = filteredCount * (g_base_thumb_size * 0.7f * masterScale) - sh + (g_base_thumb_size * 1.5f * masterScale);
                    break;
                }
            case MODE_RIVER_H:
            case MODE_WAVE:
                {
                    maxScrollX = filteredCount * (g_base_thumb_size * 0.8f * masterScale) - sw + (g_base_thumb_size * 1.5f * masterScale);
                    break;
                }
        }
        if (maxScrollY < 0) maxScrollY = 0;
        if (maxScrollX < 0) maxScrollX = 0;


        bool blockActions = isExiting || isPreviewing;
        if (!blockActions)
        {
            int key = GetKeyPressed();
            if (key != 0 && !isSearching)
            {
                TriggerEffect(g_keypressEffect, 0.4f);
            }
            if (key >= KEY_ONE && key < KEY_ONE + NUM_MODES)
            {
                g_targetMode = key - KEY_ONE;
                g_modeTransitionTimer = g_modeTransitionDuration;
            }
        }

        bool ateEscKey = false;
        if (isSearching)
        {
            int key = GetCharPressed();
            while (key > 0)
            {
                if ((key >= 32) && (key <= 125) && (searchBufferCount < 255))
                {
                    searchBuffer[searchBufferCount++] = (char)key;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) { if (searchBufferCount > 0) searchBuffer[--searchBufferCount] = '\0'; }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) { isSearching = false; if(IsKeyPressed(KEY_ESCAPE)) ateEscKey = true;}
        }
        else
        {
            if (!blockActions && IsKeyPressed(KEY_SLASH)) { isSearching = true; searchBufferCount = 0; searchBuffer[0] = '\0'; }
        }


        if (!blockActions && !isSearching && filteredCount > 0)
        {
            int direction = 0;
            keyRepeatTimer -= delta;

            bool navKeyPressed = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_L) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_H) ||
                                 IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_J) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_K);

            if (navKeyPressed && keyRepeatTimer <= 0.0f) {
                keyRepeatTimer = KEY_REPEAT_DELAY;

                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_L)) direction = 1;
                if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_H)) direction = -1;
                if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_J))
                {
                    if (g_currentMode == MODE_GRID) {
                        int cols = (GetScreenWidth() / (g_base_thumb_size * masterScale + g_base_padding * masterScale));
                        if (cols < 1) cols = 1;
                        direction = cols;
                    } else direction = 1;
                }
                if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_K))
                {
                    if (g_currentMode == MODE_GRID) {
                        int cols = (GetScreenWidth() / (g_base_thumb_size * masterScale + g_base_padding * masterScale));
                        if (cols < 1) cols = 1;
                        direction = -cols;
                    } else direction = -1;
                }

                if (direction != 0)
                {
                    int currentFilteredIdx = -1;
                    for (int i = 0; i < filteredCount; i++)
                    {
                        if (filteredIndices[i] == g_hoveredIndex)
                        {
                            currentFilteredIdx = i;
                            break;
                        }
                    }

                    int nextFilteredIdx = (currentFilteredIdx == -1) ? ((direction > 0) ? 0 : filteredCount - 1) : (currentFilteredIdx + direction);
                    nextFilteredIdx = Clamp(nextFilteredIdx, 0, filteredCount - 1);
                    g_hoveredIndex = filteredIndices[nextFilteredIdx];

                    Rectangle itemRect = {wallpapers[g_hoveredIndex].animPos.x, wallpapers[g_hoveredIndex].animPos.y, wallpapers[g_hoveredIndex].animSize.x, wallpapers[g_hoveredIndex].animSize.y};
                    if (g_currentMode == MODE_GRID || g_currentMode == MODE_RIVER_V)
                    {
                        if (itemRect.y < g_smoothScrollY) g_scroll.y = itemRect.y - g_base_padding;
                        if (itemRect.y + itemRect.height > g_smoothScrollY + sh) g_scroll.y = itemRect.y + itemRect.height - sh + g_base_padding;
                    }
                    else
                    {
                        if (itemRect.x < g_smoothScrollX) g_scroll.x = itemRect.x - g_base_padding;
                        if (itemRect.x + itemRect.width > g_smoothScrollX + sw) g_scroll.x = itemRect.x + itemRect.width - sw + g_base_padding;
                    }
                }
            }
        }


        float wheel = GetMouseWheelMove();
        if (IsKeyDown(KEY_LEFT_CONTROL))
        {
            masterScale += wheel * 0.05f;
            masterScale = Clamp(masterScale, 0.2f, 5.0f);
        }
        else if (!isPreviewing)
        {
            if(g_currentMode == MODE_RIVER_H || g_currentMode == MODE_WAVE) g_scroll.x -= wheel * 100.f;
            else g_scroll.y -= wheel * 100.f;
        }

        g_scroll.y = Clamp(g_scroll.y, 0, maxScrollY);
        g_scroll.x = Clamp(g_scroll.x, 0, maxScrollX);

        if (!isPreviewing && !isExiting && !isSearching && g_hoveredIndex != -1)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER))
            {
                isExiting = true;
                TriggerParticleBurst(GetMousePosition());
                printf("%s\n", print_filename_only ? wallpapers[g_hoveredIndex].filename : wallpapers[g_hoveredIndex].path);
                fflush(stdout);
                TriggerEffect(g_exitEffect, 1.5f);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_LEFT_SHIFT))
            {
                preview_index = g_hoveredIndex;
                closing_preview_index = g_hoveredIndex;
                atomic_store(&isFullTextureReady, false);
                g_previewStartRect = (Rectangle){wallpapers[g_hoveredIndex].animPos.x, wallpapers[g_hoveredIndex].animPos.y, wallpapers[g_hoveredIndex].animSize.x, wallpapers[g_hoveredIndex].animSize.y};
                pthread_t pt;
                pthread_create(&pt, NULL, FullPreviewLoaderThread, wallpapers[g_hoveredIndex].path);
                pthread_detach(pt);
            }
        }

        if (isPreviewing && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)))
        {
            if(IsKeyPressed(KEY_ESCAPE)) ateEscKey = true;
            if (atomic_load(&isFullTextureReady)) UnloadTexture(fullPreviewTexture);
            closing_preview_index = preview_index;
            g_previewStartRect = (Rectangle){wallpapers[closing_preview_index].animPos.x, wallpapers[closing_preview_index].animPos.y, wallpapers[closing_preview_index].animSize.x, wallpapers[closing_preview_index].animSize.y};
            preview_index = -1;
        }
        
        if (IsKeyPressed(KEY_ESCAPE) && !ateEscKey) {
            break;
        }
        
        g_smoothScrollY = Lerp(g_smoothScrollY, g_scroll.y, delta * g_anim_speed);
        g_smoothScrollX = Lerp(g_smoothScrollX, g_scroll.x, delta * g_anim_speed);

        int textures_loaded_this_frame = 0;
        for (int i = 0; i < wallpaper_count; i++)
        {
            if (textures_loaded_this_frame >= MAX_TEXTURES_TO_LOAD_PER_FRAME) break;
            if (atomic_load(&imagePending[i]))
            {
                wallpapers[i].texture = LoadTextureFromImage(pendingImages[i]);
                UnloadImage(pendingImages[i]);
                atomic_store(&wallpapers[i].loaded, true);
                atomic_store(&imagePending[i], false);
                textures_loaded_this_frame++;
            }
        }

        if (atomic_load(&fullImagePending))
        {
            fullPreviewTexture = LoadTextureFromImage(pendingFullImage);
            UnloadImage(pendingFullImage);
            atomic_store(&isFullTextureReady, true);
            atomic_store(&fullImagePending, false);
        }
        previewAnim = Lerp(previewAnim, isPreviewing ? 1.f : 0.f, delta * g_anim_speed * 0.8f);
        if (isPreviewing) { if (kenBurnsTimer < g_ken_burns_duration) kenBurnsTimer += delta; }


        BeginTextureMode(mainTarget);
        {
            ClearBackground(AppTheme.bg);
            UpdateAndDrawScene(filteredCount, filteredIndices, delta, isPreviewing, isSearching);
            if (isSearching || searchBufferCount > 0)
            {
                DrawRectangle(0, 0, sw, 40, AppTheme.overlay);
                DrawRectangleLines(0, 0, sw, 40, AppTheme.border);
                char s[512];
                snprintf(s, sizeof(s), "Search: %s", searchBuffer);
                if (isSearching && fmod(GetTime(), 1.0) > 0.5) strcat(s, "|");
                DrawText(s, 10, 10, 20, AppTheme.text);
            }
        }
        EndTextureMode();


        BeginTextureMode(bloomMaskHiRes);
        {
            ClearBackground(BLANK);
            if (g_hoveredIndex != -1 && wallpapers[g_hoveredIndex].hoverAnim > 0.01f)
            {
                Wallpaper w = wallpapers[g_hoveredIndex];
                Rectangle r = {w.animPos.x, w.animPos.y, w.animSize.x, w.animSize.y};
                if (!isPreviewing)
                    DrawRectangleRoundedLinesEx(r, 0.1f, 8, g_border_thickness_bloom * 2.f, Fade(AppTheme.border, w.hoverAnim));
            }
        }
        EndTextureMode();

        bool h = true;
        Vector2 rs = {(float)bloomMask.texture.width, (float)bloomMask.texture.height};
        BeginShaderMode(blurShader);
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "renderSize"), &rs, SHADER_UNIFORM_VEC2);
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "horizontal"), &h, SHADER_UNIFORM_INT);
        BeginTextureMode(blurPingPong);
        ClearBackground(BLANK);
        DrawTextureRec(bloomMask.texture, (Rectangle){0, 0, rs.x, -rs.y}, (Vector2){0, 0}, WHITE);
        EndTextureMode();
        h = false;
        SetShaderValue(blurShader, GetShaderLocation(blurShader, "horizontal"), &h, SHADER_UNIFORM_INT);
        BeginTextureMode(bloomMask);
        ClearBackground(BLANK);
        DrawTextureRec(blurPingPong.texture, (Rectangle){0, 0, rs.x, -rs.y}, (Vector2){0, 0}, WHITE);
        EndTextureMode();
        EndShaderMode();

        BeginDrawing();
        {
            ClearBackground(AppTheme.bg);
            BeginShaderMode(postShader);
            float tt = GetTime();
            SetShaderValue(postShader, timeLoc, &tt, SHADER_UNIFORM_FLOAT);
            Vector2 res = {(float)sw, (float)sh};
            SetShaderValue(postShader, resLoc, &res, SHADER_UNIFORM_VEC2);
            float z = 0.f, pv = 0.f; int m = 0; float sl = 0.0f;
            SetShaderValue(postShader, glitchLoc, (g_activeEffect == EFFECT_GLITCH) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, blurLoc, (g_activeEffect == EFFECT_BLUR) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            if (g_activeEffect == EFFECT_PIXELATE) { pv = 256.f * (1.f - g_effectIntensity); if (pv < 10.f) pv = 10.f; }
            SetShaderValue(postShader, pixelLoc, &pv, SHADER_UNIFORM_FLOAT);
            if (g_activeEffect == EFFECT_REVEAL) m = 3;
            SetShaderValue(postShader, mirrorLoc, &m, SHADER_UNIFORM_INT);
            SetShaderValue(postShader, shakeLoc, (g_activeEffect == EFFECT_SHAKE) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, collapseLoc, (g_activeEffect == EFFECT_REVEAL || g_activeEffect == EFFECT_COLLAPSE) ? &g_effectIntensity : &z, SHADER_UNIFORM_FLOAT);
            SetShaderValue(postShader, scanlineLoc, &sl, SHADER_UNIFORM_FLOAT);

            rlActiveTextureSlot(1); rlEnableTexture(bloomMask.texture.id);
            SetShaderValue(postShader, GetShaderLocation(postShader, "bloomTexture"), (int[]){1}, SHADER_UNIFORM_INT);
            rlActiveTextureSlot(2); rlEnableTexture(bloomMaskHiRes.texture.id);
            SetShaderValue(postShader, GetShaderLocation(postShader, "bloomTextureHiRes"), (int[]){2}, SHADER_UNIFORM_INT);

            rlActiveTextureSlot(0);
            DrawTextureRec(mainTarget.texture, (Rectangle){0, 0, (float)sw, (float)-sh}, (Vector2){0, 0}, WHITE);
            EndShaderMode();

            DrawText("Modes: 1-4 | Nav: HJKL/Arrows | Zoom: Ctrl+Scroll | Search: / | Preview: L-Shift/RMB | Select: Enter/LMB", 10, sh - 20, 10, AppTheme.text);
        }
        EndDrawing();
    }

    atomic_store(&loader_running, false);
    for (int t = 0; t < g_max_threads; t++)
    {
        pthread_join(loader_threads[t], NULL);
    }
    UnloadShader(postShader);
    UnloadShader(blurShader);
    UnloadRenderTexture(mainTarget);
    UnloadRenderTexture(bloomMask);
    UnloadRenderTexture(blurPingPong);
    UnloadRenderTexture(bloomMaskHiRes);
    for (int i = 0; i < wallpaper_count; i++)
    {
        if (atomic_load(&wallpapers[i].loaded))
        {
            UnloadTexture(wallpapers[i].texture);
        }
        free(wallpapers[i].path);
        free(wallpapers[i].filename);
    }
    CloseWindow();
    return 0;
}
