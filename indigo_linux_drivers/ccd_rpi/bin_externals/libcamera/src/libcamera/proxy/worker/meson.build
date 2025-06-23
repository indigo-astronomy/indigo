# SPDX-License-Identifier: CC0-1.0

# generate {pipeline}_ipa_proxy_worker.cpp
foreach mojom : ipa_mojoms
    worker = custom_target(mojom['name'] + '_proxy_worker',
                           input : mojom['mojom'],
                           output : mojom['name'] + '_ipa_proxy_worker.cpp',
                           depends : mojom_templates,
                           command : [
                               mojom_generator, 'generate',
                               '-g', 'libcamera',
                               '--bytecode_path', mojom_templates_dir,
                               '--libcamera_generate_proxy_worker',
                               '--libcamera_output_path=@OUTPUT@',
                               './' + '@INPUT@'
                           ],
                           env : py_build_env)

    proxy_worker_sources += worker

endforeach
