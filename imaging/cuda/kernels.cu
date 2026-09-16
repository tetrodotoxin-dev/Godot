// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

struct Complex {
  float x;
  float y;
};

extern "C" __global__ void invert_rgba8(
    const unsigned char* input,
    unsigned char* output,
    unsigned long long byte_count) {
  const unsigned long long index =
      (unsigned long long)blockIdx.x * blockDim.x + threadIdx.x;
  if (index < byte_count) {
    output[index] = (index & 3) == 3 ? input[index] : 255 - input[index];
  }
}

// All four planes have the same padded shape: RGB followed by the kernel.
// Borders filled with zeros prevent the FFT's periodic domain from wrapping source
// pixels into the opposite edge of the linear convolution.
extern "C" __global__ void pad(
    const unsigned char* input,
    Complex* planes,
    const float* weights,
    unsigned width,
    unsigned height,
    unsigned kernel_width,
    unsigned kernel_height,
    unsigned padded_width,
    unsigned padded_height) {
  const unsigned index = blockIdx.x * blockDim.x + threadIdx.x;
  const unsigned count = padded_width * padded_height;
  if (index >= count) {
    return;
  }

  const unsigned x = index % padded_width;
  const unsigned y = index / padded_width;
  for (unsigned channel = 0; channel < 3; ++channel) {
    planes[channel * count + index] = {
      x < width && y < height ? float(input[(y * width + x) * 4 + channel])
                              : 0.0f,
      0.0f};
  }

  planes[3 * count + index] = {
    x < kernel_width && y < kernel_height ? weights[y * kernel_width + x]
                                          : 0.0f,
    0.0f};
}

// The same kernel spectrum multiplies each color plane. cuFFT's inverse needs
// a 1/count normalization, which is applied here while both values are loaded.
extern "C" __global__ void multiply(Complex* planes, unsigned count) {
  const unsigned index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= 3 * count) {
    return;
  }

  const Complex image = planes[index];
  const Complex kernel = planes[3 * count + index % count];
  planes[index] = {
    (image.x * kernel.x - image.y * kernel.y) / count,
    (image.x * kernel.y + image.y * kernel.x) / count};
}

// Full convolution starts at the kernel's origin. Shifting by its half width
// and half height selects the centered output promised by the image contract.
// Alpha comes from the original image and never enters the spectral filter.
extern "C" __global__ void crop(
    const unsigned char* input,
    unsigned char* output,
    const Complex* planes,
    unsigned width,
    unsigned height,
    unsigned kernel_width,
    unsigned kernel_height,
    unsigned padded_width,
    unsigned padded_height) {
  const unsigned index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= width * height) {
    return;
  }

  const unsigned x = index % width;
  const unsigned y = index / width;
  const unsigned count = padded_width * padded_height;
  for (unsigned channel = 0; channel < 3; ++channel) {
    const float value =
        planes
            [channel * count + (y + kernel_height / 2) * padded_width + x +
             kernel_width / 2]
                .x;
    output[index * 4 + channel] = value <= 0 ? 0
                                  : value >= 255
                                      ? 255
                                      : (unsigned char)(value + 0.5f);
  }

  output[index * 4 + 3] = input[index * 4 + 3];
}

// Keeping alpha as an integer numerator postpones rounding until the final
// output. Both providers use this rule, including zero color when both inputs
// are transparent. All intermediate terms fit in an unsigned 32 bit value.
extern "C" __global__ void composite_rgba8(
    const unsigned char* background,
    const unsigned char* overlay,
    unsigned char* output,
    unsigned int count) {
  const unsigned int pixel = blockIdx.x * blockDim.x + threadIdx.x;
  if (pixel >= count) {
    return;
  }

  const unsigned int index = pixel * 4;
  const unsigned int front_alpha = overlay[index + 3];
  const unsigned int back_alpha = background[index + 3];
  const unsigned int alpha =
      front_alpha * 255 + back_alpha * (255 - front_alpha);
  for (unsigned int channel = 0; channel < 3; ++channel) {
    const unsigned int color =
        overlay[index + channel] * front_alpha * 255 +
        background[index + channel] * back_alpha * (255 - front_alpha);
    output[index + channel] = alpha ? (color + alpha / 2) / alpha : 0;
  }

  output[index + 3] = (alpha + 127) / 255;
}

// Work is indexed independently of scheduling. Each block reduces its own
// counts before adding one contribution to the retained scalar destination.
// No sample array or per-call device allocation is required.
extern "C" __global__ void sample_disk(
    unsigned int seed, unsigned int first, unsigned int count,
    unsigned long long* output) {
  unsigned int hits = 0;
  const unsigned int step = blockDim.x * gridDim.x;
  for (unsigned long long i = blockIdx.x * blockDim.x + threadIdx.x;
       i < count; i += step) {
    unsigned int value = (first + (unsigned int)i) ^ seed;
    value = ((value >> 16) ^ value) * 0x045d9f3bu;
    value = ((value >> 16) ^ value) * 0x045d9f3bu;
    value = (value >> 16) ^ value;
    const unsigned long long x = value & 65535u;
    const unsigned long long y = value >> 16;
    hits += x * x + y * y < (1ull << 32);
  }

  __shared__ unsigned int totals[256];
  totals[threadIdx.x] = hits;
  __syncthreads();
  for (unsigned int width = 128; width; width >>= 1) {
    if (threadIdx.x < width) {
      totals[threadIdx.x] += totals[threadIdx.x + width];
    }
    __syncthreads();
  }

  if (threadIdx.x == 0) {
    atomicAdd(output, (unsigned long long)totals[0]);
  }
}
