/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_AD_PLASMA_DOPPLER_PROCESSING_IMPL_H
#define INCLUDED_AD_PLASMA_DOPPLER_PROCESSING_IMPL_H


#include <gnuradio/plasma/advanced_doppler_processing.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <plasma_dsp/fft.h>

namespace gr {
namespace plasma {

    class advanced_doppler_processing_impl : public advanced_doppler_processing
    {
    private:
        af::array d_match_filt;
        af::Backend d_backend;
        size_t d_msg_queue_depth;
        int d_num_pulse_cpi;
        int d_fftsize;
    
        pmt::pmt_t d_tx_port;
        pmt::pmt_t d_rx_port;
        pmt::pmt_t d_out_port;
        pmt::pmt_t d_meta;
        pmt::pmt_t d_data;
        // Metadata keys
        pmt::pmt_t d_doppler_fft_size_key;
    
        void handle_tx_msg(pmt::pmt_t);
        void handle_rx_msg(pmt::pmt_t);
    
    public:
    advanced_doppler_processing_impl(int num_pulse_cpi, int doppler_fft_size);
        ~advanced_doppler_processing_impl();
    
        void set_msg_queue_depth(size_t) override;
        void set_backend(Device::Backend) override;
        void init_meta_dict(std::string doppler_fft_size_key) override;
    };
    

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H */
