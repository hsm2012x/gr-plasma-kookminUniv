/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "advanced_doppler_processing_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <chrono>

namespace gr {
namespace plasma {

    advanced_doppler_processing::sptr advanced_doppler_processing::make(int num_pulse_cpi, int doppler_fft_size)
    {
        return gnuradio::make_block_sptr<advanced_doppler_processing_impl>(num_pulse_cpi, doppler_fft_size);
    }
    
    
    /*
     * The private constructor
     */
    advanced_doppler_processing_impl::advanced_doppler_processing_impl(int num_pulse_cpi, int doppler_fft_size)
        : gr::block("advanced_doppler_processing",
                    gr::io_signature::make(0, 0, 0),
                    gr::io_signature::make(0, 0, 0)),
          d_num_pulse_cpi(num_pulse_cpi),
          d_fftsize(doppler_fft_size)
    {
        d_data = pmt::make_c32vector(0, 0);
    
        d_tx_port = PMT_TX;
        d_rx_port = PMT_RX;
        d_out_port = PMT_OUT;
        message_port_register_in(d_tx_port);
        message_port_register_in(d_rx_port);
        message_port_register_out(d_out_port);
        set_msg_handler(d_tx_port, [this](pmt::pmt_t msg) { handle_tx_msg(msg); });
        set_msg_handler(d_rx_port, [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
    }
    
    
    /*
     * Our virtual destructor.
     */
    advanced_doppler_processing_impl::~advanced_doppler_processing_impl() {}
    
    void advanced_doppler_processing_impl::handle_tx_msg(pmt::pmt_t msg)
    {
        if (this->nmsgs(d_tx_port) > d_msg_queue_depth)
            return;
        pmt::pmt_t samples;
        if (pmt::is_pdu(msg)) {
            samples = pmt::cdr(msg);
            d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        } else if (pmt::is_uniform_vector(msg)) {
            samples = msg;
        } else {
            GR_LOG_WARN(d_logger, "Invalid message type")
            return;
        }
        size_t n = pmt::length(samples);
        size_t io(0);
        const gr_complex* tx_data = pmt::c32vector_elements(samples, io);
    
        // Create the matched filter
        d_match_filt = af::array(af::dim4(n), reinterpret_cast<const af::cfloat*>(tx_data));
        d_match_filt = af::conjg(d_match_filt);
        d_match_filt = af::flip(d_match_filt, 0);
    }
    
    void advanced_doppler_processing_impl::handle_rx_msg(pmt::pmt_t msg)
    {
        pmt::pmt_t samples;
        if (this->nmsgs(d_rx_port) > d_msg_queue_depth or d_match_filt.elements() == 0) {
            return;
        }
        // Get a copy of the input samples
        if (pmt::is_pdu(msg)) {
            samples = pmt::cdr(msg);
            d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        } else if (pmt::is_uniform_vector(msg)) {
            samples = msg;
        } else {
            GR_LOG_WARN(d_logger, "Invalid message type")
            return;
        }
        // Compute matrix and vector dimensions
        size_t n = pmt::length(samples);
        size_t ncol = d_num_pulse_cpi;
        size_t nrow = n / ncol;
        size_t nconv = nrow + d_match_filt.elements() - 1;
        if (pmt::length(d_data) != nconv * d_fftsize)
            d_data = pmt::make_c32vector(nconv * d_fftsize, 0);
    
        // Get input and output data
        size_t io(0);
        const gr_complex* in = pmt::c32vector_elements(samples, io);
        gr_complex* out = pmt::c32vector_writable_elements(d_data, io);
    
        // Apply the matched filter to each column
        af::array rdm(af::dim4(nrow, d_num_pulse_cpi),
                      reinterpret_cast<const af::cfloat*>(in));
        rdm = af::convolve1(rdm, d_match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);
    
        // Do a doppler FFT for each range bin
        rdm = rdm.T();
        rdm = af::fftNorm(rdm, 1.0, d_fftsize);
        rdm = ::plasma::fftshift(rdm, 0);
        rdm = rdm.T();
        rdm.host(out);
    
        message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
        d_meta = pmt::make_dict();
        // init_meta_dict(pmt::symbol_to_string(d_doppler_fft_size_key));
    }
    
    void advanced_doppler_processing_impl::set_msg_queue_depth(size_t depth) { d_msg_queue_depth = depth; }
    
    void advanced_doppler_processing_impl::set_backend(Device::Backend backend)
    {
        switch (backend) {
        case Device::CPU:
            d_backend = AF_BACKEND_CPU;
            break;
        case Device::CUDA:
            d_backend = AF_BACKEND_CUDA;
            break;
        case Device::OPENCL:
            d_backend = AF_BACKEND_OPENCL;
            break;
        default:
            d_backend = AF_BACKEND_DEFAULT;
            break;
        }
        af::setBackend(d_backend);
    }
    
    void advanced_doppler_processing_impl::init_meta_dict(std::string doppler_fft_size_key)
    {
        d_doppler_fft_size_key = pmt::intern(doppler_fft_size_key);
        d_meta = pmt::make_dict();
        d_meta = pmt::dict_add(d_meta, d_doppler_fft_size_key, pmt::from_long(d_fftsize));
    }
    
    } /* namespace plasma */
    } /* namespace gr */