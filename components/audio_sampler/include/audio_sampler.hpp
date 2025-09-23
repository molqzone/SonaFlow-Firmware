#ifndef AUDIO_SAMPLER_HPP_
#define AUDIO_SAMPLER_HPP_

#include <cstddef>

#include "driver/i2s_std.h"
#include "driver/i2s_types.h"

/**
 * @class AudioSampler
 * @brief A class to manage on I2S microphone input channel.
 */
class AudioSampler {
   public:
    /**
   * @brief Construct a new AudioSampler object
   */
    AudioSampler();

    /**
   * @brief Destroy the AudioSampler object
   */
    ~AudioSampler();

    /**
   * @brief Initializes the I2S hardware. This method must be called and checked
   * for success before any other operations are performed.
   * @return esp_err_t ESP_OK on success, or an error code on failure.
   */
    esp_err_t init();

    /**
   * @brief Reads a block of audio samples from the microphone.
   *
   * @param[out] buffer Pointer to the destination buffer where audio data will
   * be stored.
   * @param[in]  buffer_size The size of the destination buffer in bytes.
   * @param[out] bytes_read Pointer to a variable where the number of bytes
   * actually read will be stored. This can be less than buffer_size if a
   * timeout occurs.
   * @return esp_err_t ESP_OK on success, or an ESP-IDF error code on failure.
   */
    esp_err_t read(int16_t* dest, size_t samples, size_t* samples_read);

    // Delete the copy constructor and copy assignment operator.
    // An AudioSampler instance represents a unique hardware resource and cannot
    // be copied.
    AudioSampler(const AudioSampler&) = delete;
    AudioSampler& operator=(const AudioSampler&) = delete;

    // Declare the move constructor.
    // Transfers ownership of the I2S handle from another AudioSampler instance.
    AudioSampler(AudioSampler&& other) noexcept;
    // Declare the move assignment operator.
    // Transfers ownership of the I2S handle from another AudioSampler instance.
    AudioSampler& operator=(AudioSampler&& other) noexcept;

   private:
    /**
   * @brief Private helper function to release all acquired I2S resources.
   *
   * This function checks if the handle is valid, disables the channel,
   * deletes it, and logs any errors encountered during the process.
   * It ensures cleanup logic is centralized and consistent.
   */
    void releaseResources();

    /**
   * @brief Handle for the configured I2S receive channel.
   *
   * This handle is the unique identifier for the hardware resource managed
   * by this class instance. It's initialized in the constructor and released
   * in the destructor. A null or invalid handle indicates an empty/moved-from
   * state.
   */
    i2s_chan_handle_t rx_handle_;
};

#endif  // AUDIO_SAMPLER_HPP_