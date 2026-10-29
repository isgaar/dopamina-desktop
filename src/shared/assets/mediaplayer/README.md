# Windows Media Player Legacy — Extracted Resources & Visualizations

Este repositorio contiene la extracción completa de recursos del **Windows Media Player Legacy** (Reproductor de Windows Media clásico), inspeccionados y recuperados utilizando la herramienta **REA (Reverse Engineer Anything)** y analizados a nivel PE (*Portable Executable*).

---

## 📊 Resumen de Recursos Extraídos

Total de archivos extraídos: **230+**

| Categoría | Cantidad | Ubicación | Descripción |
| :--- | :---: | :--- | :--- |
| 🌌 **Visualizaciones Clásicas** | **16+** | `visualizations/` | Motores DLL (Alchemy & Effects), texturas de partículas, bibliotecas de tipos COM (`.tlb`), interfaz `IWMPEffects.idl`, catálogo de presets y reproductor web interactivo. |
| 🎨 **Iconos (.ico)** | **19** | `icons/` | Iconos multi-resolución de alta fidelidad (16x16 hasta 256x256 píxeles a 32bpp). |
| 🖼️ **Bitmaps (.bmp)** | **35** | `bitmaps/` | Bitmaps de interfaz de usuario, botones, fondos y controles. |
| 🌐 **Páginas HTML (.html)** | **26** | `html/` | 26 páginas HTML integradas de metadatos, álbumes, DVD y diálogo de tienda. |
| 📐 **DirectUI XML (.xml)** | **1** | `ui_directui/` | Hojas de estilo y definiciones de diseño DirectUI de Windows Media Player. |
| ⚙️ **Perfiles de Codificación (.prx)** | **25** | `encoding_profiles/` | Perfiles XML de audio y video para codificación y sincronización de dispositivos. |
| 📜 **Scripts de Registro (.rgs)** | **76** | `registry_scripts/` | Scripts ATL para registro de CLSIDs, interfaces COM y extensiones. |
| 🛡️ **Manifiestos (.manifest)** | **10** | `manifests/` | Manifiestos de activación de componentes y privilegios de ejecución. |
| 🔤 **Tablas de Cadenas (.json)** | **3** | `string_tables/` | Diccionarios completos de textos, menús y mensajes traducidos. |
| 📡 **Media Renderer & UPnP** | **18** | `media_renderer_assets/`, `network_sharing_assets/` | Esquemas XML UPnP/DLNA y logos/iconos de transmisión en red. |

---

## 🌌 Visualizaciones Extraídas (`visualizations/`)

Se recuperaron todos los componentes que componen el sistema de visualizaciones del reproductor:

1. **Motores Binarios (`visualizations/binaries/`)**:
   - `mpvis_x64.dll` y `mpvis_x86.dll`: Motor de visualizaciones **Alquimia (Alchemy)** (64-bit y 32-bit).
   - `wmpeffects_x64.dll` y `wmpeffects_x86.dll`: Motor de efectos y ecualizadores de Windows Media Player.
   - `mpvis_es-MX.dll.mui`: Módulo de localización con los nombres de presets en español.
2. **Texturas y Mapas de Partículas (`visualizations/textures/`)**:
   - Texturas de decaimiento mipmap (64x64, 128x128, 256x256, 512x512 y 1024x1024) extraídas de `mpvis.DLL` usadas para el renderizado de estelas y ondas psicodélicas.
3. **Bibliotecas de Tipos COM (`visualizations/typelibs/`)**:
   - `mpvis.tlb` y `wmp.tlb`: TypeLibs binarios con las interfaces COM y contratos de renderizado.
4. **Contratos COM e IDL (`visualizations/com_registry/`)**:
   - `IWMPEffects.idl`: Definición IDL de las interfaces `IWMPEffects` y `IWMPEffects2` (`Render`, `TimedLevel`, etc.).
   - `mpvis_alchemy_registration.rgs` y `wmp_bars_and_waves_registration.rgs`: Scripts de registro del sistema.
5. **Catálogo de Presets (`visualizations/presets_catalog.json`)**:
   - Catálogo de todas las colecciones (**Alquimia**, **Barras y ondas**, **Puntas**, **Partículas**, **Ambiente/Batería** y **Plenoptic**) y sus más de 40 presets.
6. **Reproductor Web Interactivo (`visualizations/web_player/`)**:
   - Implementación web en HTML5 Canvas y Web Audio API con la interfaz fiel de Windows Media Player Legacy (botón orbe azul, barra de volumen, timeline y selector de presets).
   - Recrea en tiempo real **WonderWave** (la visualización exacta de la captura), **SuperStar**, **SwitchBlur**, **Barras y ondas**, **Bruma marina**, **Tormenta de fuego**, **Puntas** y **Ambiente**, reaccionando al audio en vivo o archivos MP3 cargados por el usuario.

---

## 📁 Estructura del Directorio

```
mediaplayer/
├── visualizations/           # Módulo completo de visualizaciones
│   ├── binaries/             # DLLs x64/x86 de Alchemy y WMP Effects
│   ├── textures/             # Texturas de partículas (64x64 a 1024x1024)
│   ├── typelibs/             # TypeLibs COM (.tlb)
│   ├── com_registry/         # Scripts RGS e IWMPEffects.idl
│   ├── presets_catalog.json  # Catálogo JSON de presets y CLSIDs
│   └── web_player/           # Reproductor interactivo en HTML5/Canvas
├── icons/                    # Archivos .ico completos (wmplayer, wmp, setup_wm, etc.)
├── bitmaps/                  # Archivos .bmp (setup_wm, mpvis, WmpDui)
├── html/                     # Páginas web embebidas (BLOCKED_ALBUMINFO, etc.)
├── ui_directui/              # Definiciones DirectUI XML
├── encoding_profiles/        # Perfiles de codificación WMP en XML
├── registry_scripts/         # Scripts ATL .rgs
├── manifests/                # Manifiestos de aplicación
├── string_tables/            # Tablas de cadenas localizadas en formato JSON
├── media_renderer_assets/    # XMLs y badges de Media Renderer
├── network_sharing_assets/   # XMLs y badges de Network Sharing
├── RESOURCE_CATALOG.json     # Manifiesto estructurado en JSON de todos los recursos
└── README.md                 # Este documento
```

---

## 🛠️ Herramientas Utilizadas
- **REA (Reverse Engineer Anything)**: Extracción e inspección precisa de tablas de recursos PE32/PE32+ (`inspect-pe-resources`, `inspect-native-exports`).
- **Antigravity Customization**: Integración oficial del MCP server de REA y skill `reverse-engineer-anything` para Antigravity IDE.
