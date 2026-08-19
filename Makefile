PYTHON ?= python3
BOOTSTRAP_STAMP := .venv/.world_sim_bootstrap
BOOTSTRAP_INPUTS := requirements-dev.txt tools/toolchain.lock.json tools/bootstrap.py tools/godot_runtime.py CMakePresets.json cmake/Dependencies.cmake

.PHONY: bootstrap build play check smoke

$(BOOTSTRAP_STAMP): $(BOOTSTRAP_INPUTS)
	$(PYTHON) tools/bootstrap.py
	@$(PYTHON) -c "from pathlib import Path; Path('$(BOOTSTRAP_STAMP)').touch()"

bootstrap: $(BOOTSTRAP_STAMP)

build: bootstrap
	$(PYTHON) tools/dev.py build --preset dev

play: build
	$(PYTHON) tools/dev.py run

check: bootstrap
	$(PYTHON) tools/dev.py check --preset dev

smoke: build
	$(PYTHON) tools/dev.py play --scenario smoke
