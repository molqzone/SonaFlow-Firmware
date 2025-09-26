#ifndef AUDIO_SOURCE_HPP_
#define AUDIO_SOURCE_HPP_

#include <cstdint>
#include <memory>
#include <span>

#include "driver/i2s_std.h"
#include "driver/i2s_types.h"

/**
 * @class AudioSampler
 * @brief A class to manage on I2S microphone input channel.
 */
class AudioSource {
   public:
    /**
     * @brief Creates and initializes an AudioSampler instance.
     *
     * This factory method handles all I2S peripheral configuration and
     * returns a fully initialized object.
     *
     * @return A std::unique_ptr containing the AudioSampler on success,
     * or nullptr on failure.
     */
    static std::unique_ptr<AudioSource> Create();

    /**
   * @brief Destroy the AudioSampler object
   */
    ~AudioSource();

    /**
   * @brief Reads a block of audio samples from the microphone.
   *
   * @param[out] dest Pointer to the destination buffer where samples will be stored.
   * The buffer must be large enough to hold 'samples' int16_t values.
   * @param[in]  samples The number of int16_t samples to read into the dest buffer.
   * @param[out] samples_read Pointer to a variable where the number of samples
   * actually read will be stored. This can be less than 'samples' if a timeout
   * occurs.
   * @return esp_err_t ESP_OK on success, or an ESP-IDF error code on failure.
   */
    esp_err_t Read(std::span<int16_t> dest_buffer, size_t& samples_read);

    /**
     * @brief Reads a frame of audio and calculates its feature.
     *
     * This method reads a predefined number of samples, computes a feature
     * (e.g., RMS volume), and scales it to an int8_t value. This is the
     * primary method for real-time audio sensing.
     *
     * @param[out] feature The calculated and scaled audio feature.
     * @return esp_err_t ESP_OK on success, or an error code on failure.
     */
    esp_err_t GetFeature(int8_t& feature);

    // Delete the copy constructor and copy assignment operator.
    // An AudioSampler instance represents a unique hardware resource and cannot
    // be copied.
    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;

    // Declare the move constructor.
    // Transfers ownership of the I2S handle from another AudioSampler instance.
    AudioSource(AudioSource&& other) noexcept;
    // Declare the move assignment operator.
    // Transfers ownership of the I2S handle from another AudioSampler instance.
    AudioSource& operator=(AudioSource&& other) noexcept;

   private:
    /**
     * @brief Private constructor to enforce creation via the factory method.
     * @param handle An already initialized I2S channel handle.
     */
    explicit AudioSource(i2s_chan_handle_t handle);

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

#endif  // AUDIO_SOURCE_HPP_