# GraphicEngine

Prototipo de renderizado voxel en C++17 y OpenGL 3.3. Renderiza una cuadrícula
5 × 5 × 5 mediante instancing: los 125 voxels se dibujan con una sola llamada.

## Controles

- `W` / `S`: avanzar y retroceder.
- `A` / `D`: desplazarse lateralmente.
- Flechas arriba / abajo: subir y bajar.
- Mantener clic derecho y mover el mouse: rotar la cámara.
- `Esc`: cerrar.

El movimiento usa tiempo real (`deltaTime`), por lo que no depende de los FPS.

## Requisitos

- Visual Studio 2022 con el toolset C++ `v143`.
- OpenGL 3.3 o posterior.
- vcpkg integrado con Visual Studio (modo clásico).
- Dependencias: `glew`, `glfw3` y `glm`.

## Compilación

Abra `GraphicEngine.sln`, seleccione `x64` y compile Debug o Release. Desde una
Developer PowerShell también puede ejecutar:

```powershell
msbuild GraphicEngine.sln /p:Configuration=Debug /p:Platform=x64
```

## Arquitectura

- `Engine/Core`: aplicación, ventana, entrada y reloj del motor.
- `Engine/Renderer`: renderer, shaders externos, cámara, mallas y texturas.
- `Engine/Scene`: entidades y componentes de transformación y voxel.
- `Sandbox`: aplicación de demostración; no contiene internals del motor.

Los cambios de tamaño actualizan el viewport y la relación de aspecto. Los
recursos OpenGL se destruyen antes que la ventana y el contexto.

La demo no renderiza objetos hardcodeados desde `main`: crea una entidad
`Voxel World`, le agrega `TransformComponent` y `VoxelGridComponent`, y el
renderer procesa la escena.

Los voxels son mallas sólidas con normales, material por entidad, eliminación de
caras traseras e iluminación direccional definida desde la propia escena.

`Mesh` acepta geometría genérica con posición, normal, UV e índices; el cubo es
solo una fábrica de geometría usada por la demo. Sus matrices se siguen enviando
por instancing. Los shaders de `Assets/Shaders` se copian junto al ejecutable y
se cargan en tiempo de ejecución. `Texture2D` encapsula los recursos y slots de
textura OpenGL y puede cargar imágenes PPM desde disco.

`AssetManager` conserva una sola instancia de cada shader, textura y malla bajo
un nombre estable. `MeshRendererComponent` permite que una entidad solicite una
malla genérica; la demo incluye un cubo naranja independiente para validar este
camino, mientras `VoxelGridComponent` sigue generando sus instancias en lote.

## Escenas

`SceneSerializer` carga y guarda el formato de texto versionado `.gescene`. La
demo se define en `Assets/Scenes/Sandbox.gescene`, no en el código de Sandbox.
Las entidades usan IDs persistentes y pueden tener padre; `Scene` compone sus
transformaciones locales, rechaza ciclos y valida referencias inválidas.

## Editor

Dear ImGui está integrado en `ThirdParty/imgui`. El editor muestra jerarquía de
escena, inspector de componentes y estadísticas de frame. Desde el inspector se
pueden modificar transformaciones, materiales, grids voxel y luces en tiempo
real; `Save Scene` serializa el estado actual al archivo `.gescene` cargado.

La escena se renderiza en un `Framebuffer` independiente con attachments de
color y profundidad. La textura resultante aparece dentro del panel `Viewport`,
que puede acoplarse y redimensionarse junto con los demás paneles. La cámara solo
captura controles cuando el cursor está sobre este viewport.

El framebuffer también contiene un attachment entero `R32UI` invisible. El
renderer escribe ahí el ID de cada entidad, permitiendo seleccionarla con clic
izquierdo directamente desde `Scene View`; jerarquía e inspector se sincronizan
con el resultado.

## Mundo voxel

`Engine/Voxel` contiene tipos de bloque, datos de `Chunk` y construcción de
mallas. El primer terreno usa chunks de 16×32×16, generación procedural por seed
y capas Grass/Dirt/Stone. `ChunkMeshBuilder` emite únicamente caras junto a aire,
evitando renderizar geometría interna. El atlas `Blocks.ppm` agrupa las texturas
de bloque y `VoxelTerrainComponent` permite persistir la seed en la escena.

El menú `Game > Play` o `F5` activa interacción y muestra una mira. El renderer
mantiene el `Chunk` editable, realiza raycast desde la cámara hasta 8 bloques y
reconstruye la malla tras cada cambio. En Play, clic izquierdo rompe y
`Shift + clic izquierdo` coloca un bloque de tierra; clic derecho controla la
vista.

En Play, `PlayerController` reemplaza el vuelo libre por un cuerpo AABB de
0.6×1.8 bloques. Aplica gravedad con substeps, colisión separada por ejes,
detección de suelo, salto con `Space` y respawn al caer fuera del mundo. Al
entrar en Play, el jugador aparece sobre el bloque más alto del centro del chunk.

`VoxelWorld` administra chunks mediante coordenadas globales firmadas y resuelve
correctamente posiciones negativas. La escena inicial genera un área 3×3 de
chunks; la altura usa coordenadas mundiales para mantener continuidad y el
mesher consulta chunks vecinos para eliminar caras ocultas en sus fronteras.

El streaming usa la posición de la cámara/jugador para garantizar un cuadrado de
chunks alrededor suyo. `View radius` se ajusta desde el inspector entre 1 y 4.
Los chunks visitados permanecen cargados para conservar ediciones; al conectar
un chunk nuevo solo se remeshean sus fronteras y vecinos inmediatos.

El meshing se ejecuta en hasta cuatro workers mediante snapshots inmutables que
incluyen los bordes vecinos. Cada chunk tiene una revisión: los resultados de
jobs viejos se descartan si el mundo cambió mientras trabajaban. La creación y
actualización de buffers OpenGL permanece en el hilo principal.

Los chunks modificados se guardan en binario bajo `Saves/World_<seed>` junto al
ejecutable. Cada archivo incluye magic, versión y conteo de bloques. Los chunks
que quedan más allá del radio visible más un margen se guardan y descargan; al
regresar se restauran antes de remeshear. El cierre del mundo salva cualquier
cambio sucio restante.

## Diagnóstico de rendimiento

`F2` o `Debug > Triangle Wireframe` alterna el rasterizado de líneas para mostrar
los triángulos reales. El panel Statistics reporta triángulos enviados, draw
calls, chunks visibles y jobs de meshing pendientes. Estas métricas permiten
comparar optimizaciones bajo la misma cámara y distancia de chunks.

`ChunkMeshBuilder` usa greedy meshing en los tres ejes. Las caras visibles con
la misma orientación y material se fusionan en rectángulos grandes; las UV se
repiten mediante un índice de tile separado para conservar el pixel art. Esto
reduce triángulos sin volver al coste de instanciar un cubo completo por bloque.

`F3` o `Debug > Frustum Culling` alterna el descarte de chunks fuera de cámara.
El frustum se extrae de view-projection y prueba las ocho esquinas del AABB
transformado de cada chunk. Statistics separa chunks cargados de visibles para
mostrar cuántos draw calls y triángulos fueron evitados.
