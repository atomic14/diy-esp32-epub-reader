#pragma once

#include <esp_log.h>

#include "miniz.h"

#define TAG "ZIP"

class ZipFile
{
private:
  const char *m_filename;

public:
  ZipFile(const char *filename)
  {
    m_filename = filename;
  }
  // read a file from the zip file allocating the required memory for the data
  const char *read_file_to_memory(const char *filename)
  {
    // open up the epub file using miniz
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    bool status = mz_zip_reader_init_file(&zip_archive, m_filename, 0);
    if (!status)
    {
      ESP_LOGE(TAG, "mz_zip_reader_init_file() failed!\n");
      ESP_LOGE(TAG, "Error %s\n", mz_zip_get_error_string(zip_archive.m_last_error));
      return nullptr;
    }
    // Run through the archive and find the requiested file
    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
      mz_zip_archive_file_stat file_stat;
      if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
      {
        ESP_LOGE(TAG, "mz_zip_reader_file_stat() failed!\n");
        ESP_LOGE(TAG, "Error %s\n", mz_zip_get_error_string(zip_archive.m_last_error));
        mz_zip_reader_end(&zip_archive);
        return nullptr;
      }
      // is this the file we're looking for?
      if (strcmp(filename, file_stat.m_filename) == 0)
      {
        ESP_LOGI(TAG, "Extracting %s\n", file_stat.m_filename);
        // allocate memory for the file
        size_t file_size = file_stat.m_uncomp_size;
        char *file_data = (char *)malloc(file_size + 1);
        file_data[file_size] = 0;
        if (!file_data)
        {
          ESP_LOGE(TAG, "Failed to allocate memory for %s\n", file_stat.m_filename);
          mz_zip_reader_end(&zip_archive);
          return nullptr;
        }
        // read the file
        status = mz_zip_reader_extract_to_mem(&zip_archive, i, file_data, file_size, 0);
        if (!status)
        {
          ESP_LOGE(TAG, "mz_zip_reader_extract_to_mem() failed!\n");
          ESP_LOGE(TAG, "Error %s\n", mz_zip_get_error_string(zip_archive.m_last_error));
          free(file_data);
          mz_zip_reader_end(&zip_archive);
          return nullptr;
        }
        // Close the archive, freeing any resources it was using
        mz_zip_reader_end(&zip_archive);
        ESP_LOGI(TAG, "Extracted data");
        return file_data;
      }
    }
    ESP_LOGE(TAG, "Could not find file %s", filename);
    mz_zip_reader_end(&zip_archive);
    return nullptr;
  }
  bool read_file_to_file(const char *filename, const char *dest)
  {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    bool status = mz_zip_reader_init_file(&zip_archive, m_filename, 0);
    if (!status)
    {
      ESP_LOGE(TAG, "mz_zip_reader_init_file() failed!\n");
      ESP_LOGE(TAG, "Error %s\n", mz_zip_get_error_string(zip_archive.m_last_error));
      return false;
    }
    bool res = mz_zip_reader_extract_file_to_file(&zip_archive, filename, dest, 0);
    mz_zip_reader_end(&zip_archive);
    if (!res)
    {
      ESP_LOGE(TAG, "mz_zip_reader_extract_file_to_file() %s to %s failed!\n", filename, dest);
    }
    return res;
  }

  ~ZipFile()
  {
  }
};