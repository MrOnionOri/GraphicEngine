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
