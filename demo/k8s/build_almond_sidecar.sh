build_almond_sidecar() {
  local overlays_dir="$1"
  local image_id

  docker buildx build --platform linux/amd64 --load \
    -f "$overlays_dir/almond-base/Dockerfile" \
    -t almond-sidecar:local "$overlays_dir"

  image_id="$(docker image inspect --format='{{.Id}}' almond-sidecar:local)"
  ALMOND_SIDECAR_IMAGE="almond-sidecar:local-${image_id#sha256:}"
  docker image tag almond-sidecar:local "$ALMOND_SIDECAR_IMAGE"
}
