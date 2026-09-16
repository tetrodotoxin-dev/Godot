// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// This is project source. The CUDA provider compiles it without knowing which
// operations its entries implement. The project policy supplies those promises.
extern "C" __global__ void invert(const unsigned char* input, unsigned char* output, unsigned count) {
  unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) output[i] = (i % 4 == 3) ? input[i] : 255 - input[i];
}

extern "C" __global__ void posterize(const unsigned char* input, unsigned char* output, unsigned count, unsigned levels) {
  unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) output[i] = (i % 4 == 3) ? input[i] : ((input[i] * (levels - 1) + 127) / 255) * 255 / (levels - 1);
}

extern "C" __global__ void convolve(const unsigned char* input, unsigned char* output,
    unsigned width, unsigned height, const float* weights, unsigned kw, unsigned kh) {
  unsigned pixel = blockIdx.x * blockDim.x + threadIdx.x;
  if (pixel >= width * height) return;
  unsigned x = pixel % width, y = pixel / width;
  for (unsigned channel = 0; channel != 3; ++channel) {
    float sum = 0;
    for (unsigned ky = 0; ky != kh; ++ky) {
      int sy = int(y) + int(kh / 2) - int(ky);
      if (sy < 0 || sy >= height) continue;
      for (unsigned kx = 0; kx != kw; ++kx) {
        int sx = int(x) + int(kw / 2) - int(kx);
        if (sx >= 0 && sx < width) sum += input[(sy * width + sx) * 4 + channel] * weights[ky * kw + kx];
      }
    }
    output[pixel * 4 + channel] = (unsigned char)fminf(255.0f, fmaxf(0.0f, floorf(sum + 0.5f)));
  }
  output[pixel * 4 + 3] = input[pixel * 4 + 3];
}

extern "C" __global__ void composite(const unsigned char* back, const unsigned char* front,
    unsigned char* output, unsigned count) {
  unsigned pixel = blockIdx.x * blockDim.x + threadIdx.x;
  if (pixel >= count) return;
  unsigned i = pixel * 4;
  unsigned a = front[i + 3], b = back[i + 3];
  unsigned alpha = a * 255 + b * (255 - a);
  for (unsigned c = 0; c != 3; ++c) {
    unsigned color = front[i + c] * a * 255 + back[i + c] * b * (255 - a);
    output[i + c] = alpha ? (color + alpha / 2) / alpha : 0;
  }
  output[i + 3] = (alpha + 127) / 255;
}
