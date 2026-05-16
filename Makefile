SHELL := /bin/bash
ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BACKEND_DIR := $(ROOT_DIR)backend
BACKEND_BUILD_DIR := $(BACKEND_DIR)/build
FRONTEND_DIR := $(ROOT_DIR)frontend/app
COMPOSE_FILE := $(ROOT_DIR)docker-compose.yml

.PHONY: help clean infra-up infra-down backend-configure backend-build backend-run backend-test frontend-install frontend-dev frontend-test wasm-build db-migrate parser-test perf ci docker-build deploy-plan

help:
	@echo "Nexustal command index"
	@echo "  make clean               Remove backend build artifacts"
	@echo "  make infra-up            Start PostgreSQL and Redis"
	@echo "  make infra-down          Stop the self-hosted stack"
	@echo "  make backend-configure   Configure backend build tree"
	@echo "  make backend-build       Build nexustal_engine"
	@echo "  make backend-run         Run the local engine bootstrap"
	@echo "  make backend-test        Run backend tests"
	@echo "  make frontend-install    Install frontend dependencies"
	@echo "  make frontend-dev        Start Nuxt dev server"
	@echo "  make frontend-test       Run frontend test suite"
	@echo "  make wasm-build          Build Wasm libraries"
	@echo "  make db-migrate         List runtime SQL migrations"
	@echo "  make parser-test        Placeholder parser tests"
	@echo "  make perf               Placeholder performance suite"
	@echo "  make ci                 Run the local CI sequence"

clean:
	rm -rf $(BACKEND_BUILD_DIR)

infra-up:
	@command -v docker >/dev/null || { echo "docker is required"; exit 1; }
	docker compose -f $(COMPOSE_FILE) up -d --build

infra-down:
	@command -v docker >/dev/null || { echo "docker is required"; exit 1; }
	docker compose -f $(COMPOSE_FILE) down

backend-configure:
	@command -v cmake >/dev/null || { echo "cmake is required"; exit 1; }
	@command -v conan >/dev/null || { echo "conan is required"; exit 1; }
	conan install $(BACKEND_DIR) --output-folder=$(BACKEND_BUILD_DIR) --build=missing -s build_type=Debug
	cmake -S $(BACKEND_DIR) -B $(BACKEND_BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=$(BACKEND_BUILD_DIR)/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug

backend-build: backend-configure
	cmake --build $(BACKEND_BUILD_DIR) --parallel

backend-run: backend-build
	$(BACKEND_BUILD_DIR)/nexustal_engine

backend-test: backend-configure
	@command -v ctest >/dev/null || { echo "ctest is required"; exit 1; }
	cmake --build $(BACKEND_BUILD_DIR) --parallel
	ctest --test-dir $(BACKEND_BUILD_DIR) --output-on-failure

frontend-install:
	@command -v npm >/dev/null || { echo "npm is required"; exit 1; }
	@if [ ! -f "$(FRONTEND_DIR)/package.json" ]; then echo "Nuxt app is not initialized yet"; exit 1; fi
	cd $(FRONTEND_DIR) && npm install

frontend-dev:
	@command -v npm >/dev/null || { echo "npm is required"; exit 1; }
	@if [ ! -f "$(FRONTEND_DIR)/package.json" ]; then echo "Nuxt app is not initialized yet"; exit 1; fi
	cd $(FRONTEND_DIR) && npm run dev

frontend-test:
	@echo "TODO: run frontend tests"

wasm-build:
	@echo "TODO: compile shared C++ libraries to Wasm"

db-migrate:
	@find $(BACKEND_DIR)/migrations -maxdepth 1 -type f -name '*.sql' | sort

parser-test:
	@echo "TODO: run parser regression tests"

perf:
	@echo "TODO: execute performance benchmark suite"

ci: backend-build backend-test frontend-test parser-test perf
	@echo "Local CI sequence finished"

docker-build:
	@command -v docker >/dev/null || { echo "docker is required"; exit 1; }
	docker build -t nexustal/engine:latest -f $(BACKEND_DIR)/Dockerfile $(ROOT_DIR)

deploy-plan:
	@echo "TODO: add deployment planning artifacts"
