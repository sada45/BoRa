void TIMER1_IRQHandler() {
	*ttt = T1TC;
	// times[times_counter] = T1TC;
	if (T1IR & TIR_MR0_Interrupt) {
		next_int_counter += freq_chg_int;
		reminder += freq_chg_dec;
		if (reminder >= CHIRP_TIME_ACC) {
			reminder -= CHIRP_TIME_ACC;
			next_int_counter++;
		}
		T1MR0 = next_int_counter;
		T1IR = TIR_MR0_Interrupt;
		
		/* It would be more clear if we use a array or use bit shift
		   such as the function cc2400_spi_continue_write()
		   but this way is much faster and can save stack memory! 
		   The time consumption is about 1.84us, while the array-based takes 3.2us 
		 */
		if (write_val & 128) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 64) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 32) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 16) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 8) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 4) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 2) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		if (write_val & 1) MOSI_SET;
		else MOSI_CLR;
		SCLK_SET;
		SCLK_CLR;
		cur_freq_bin++;
		if (is_qua) {
			RXLED_CLR;
			is_qua = 0;
		}
		else {
			RXLED_SET;
			is_qua = 1;
		}
		if (cur_freq_bin == max_freq_chg_num) {
			/* Start the next chirp */
			cur_chirp++;
			cur_freq_bin = 0;
			if (cur_chirp == QUA_START_LEN) {
				freq_chg_int = QUA_FREQ_CHG_INT;
				freq_chg_dec = QUA_FREQ_CHG_DEC;
				max_freq_chg_num = QUA_MAX_FREQ_CHG_NUM;
			}
			else if (cur_chirp == DATA_START_LEN) {
				// Recover the frequency change interval
				freq_chg_int = FREQ_CHG_INT;
				freq_chg_dec = FREQ_CHG_DEC;
				max_freq_chg_num = MAX_FREQ_CHG_NUM;
			}
			else if (cur_chirp == chirp_num) {
				/* Transmission done, disable timer interrupt,
				   close the RF in the main program */
				T1TCR = TCR_Counter_Reset;
				ICER0 = ICER0_ICE_TIMER1;
				t_int = 1;
				return;
			}
			write_seq = mdmctrl_seq[start_freq[cur_chirp]];
			
		}
		write_val = write_seq[cur_freq_bin];
		end_times[times_counter] = next_int_counter;
		spi_times[times_counter++] = T1TC;
		ttt++;
	}
}
