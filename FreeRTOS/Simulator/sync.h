// Exposes c++ synchronism methods

#ifndef SYNC_H
	#define SYNC_H


	#if defined __cplusplus
	extern "C" {
	#endif

		void signal_memory_log_finished();
		void wait_before_start();


	#if defined __cplusplus
	}
	#endif
#endif /* SYNC_H */