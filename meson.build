project(
    'penrose-example',
    'cpp',
    version: '0.1',
    default_options: [
        'warning_level=3',
        'cpp_std=c++20'
    ])

src = [
    'src/Main.cpp'
]

executable('penrose_example', src)

# shaders compilation
shaders = [
    'data/shaders/default-forward-rendering.vert',
    'data/shaders/default-forward-rendering.frag',
]

foreach shader : shaders
    message('processing shader ' + shader)
    run_command('glslangValidator', '-gVS', '-V', shader, '-o', shader + '.spv', check: true)
endforeach
