/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_AD_PLASMA_DOPPLER_PROCESSING_H
#define INCLUDED_AD_PLASMA_DOPPLER_PROCESSING_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>
#include <gnuradio/plasma/device.h>
// #include "device.h"

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API advanced_doppler_processing : virtual public gr::block
{
public:
    typedef std::shared_ptr<advanced_doppler_processing> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::doppler_processing.
     *
     * To avoid accidental use of raw pointers, plasma::doppler_processing's
     * constructor is in a private implementation
     * class. plasma::doppler_processing::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t num_pulse_cpi, size_t nfft);

    virtual void set_msg_queue_depth(size_t) = 0;

    virtual void set_backend(Device::Backend) = 0;

    virtual void set_metadata_keys(const std::string& n_pulse_cpi_key,
                                   const std::string& doppler_fft_size_key) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_H */
