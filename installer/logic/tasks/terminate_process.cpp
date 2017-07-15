#include "stdafx.h"
#include "terminate_process.h"
#include "../tools.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include "../../../gui/constants.h"
#endif //_WIN32

namespace installer
{
	namespace logic
	{
		bool get_process_id(const QString& _socket_name, unsigned int& _process_id)
		{
			int timeout = 5000;
			QLocalSocket socket;

			bool conn_ok = false;

			for (int i = 0; i < 2; ++i) 
			{
				socket.connectToServer(_socket_name);
				conn_ok = socket.waitForConnected(timeout/2);
				if (conn_ok || i)
					break;

				int ms = 250;
				::Sleep(DWORD(ms));
			}

			if (conn_ok)
			{
				QByteArray msg(QString(crossprocess_message_get_process_id).toUtf8());
				QDataStream ds(&socket);
				ds.writeBytes(msg.constData(), msg.size());

				if (socket.waitForBytesWritten(timeout))
				{
					if (socket.waitForReadyRead(timeout))
					{
						QByteArray data_read = socket.readAll();
						if (data_read.size() == sizeof(_process_id))
						{
							_process_id = *(unsigned int*) data_read.data();

							return true;
						}
					}
				}
			}

			return false;
		}

		bool post_shutdown(const QString& _socket_name)
		{
			int timeout = 5000;
			QLocalSocket socket;

			bool conn_ok = false;

			for (int i = 0; i < 2; ++i) 
			{
				socket.connectToServer(_socket_name);
				conn_ok = socket.waitForConnected(timeout/2);
				if (conn_ok || i)
					break;

				int ms = 250;
				::Sleep(DWORD(ms));
			}

			if (conn_ok)
			{
				QByteArray msg(QString(crossprocess_message_shutdown_process).toUtf8());
				QDataStream ds(&socket);
				ds.writeBytes(msg.constData(), msg.size());

				if (socket.waitForBytesWritten(timeout))
				{
					if (socket.waitForReadyRead(timeout))
						return (socket.read(qstrlen("icq")) == "icq");
				}
			}

			return false;
		}

		bool connect_to_process_exit_it(const QString& _socket_name)
		{
			bool res = false;

			unsigned int process_id = 0;
			if (get_process_id(_socket_name, process_id))
			{
				HANDLE process = ::OpenProcess(SYNCHRONIZE, FALSE, process_id);
				if (process)
				{
					if (post_shutdown(_socket_name))
					{
						if (::WaitForSingleObject(process, 10000) != WAIT_TIMEOUT)
							res = true;
					}
				}
			}

			return res;
		}

		bool abnormal_terminate()
		{
			bool res = false;

#ifdef _WIN32

			DWORD processes_ids[1024], needed, count_processes;
			
			QString path_installed = get_installed_product_path();

			if (::EnumProcesses(processes_ids, sizeof(processes_ids), &needed))
			{
				count_processes = needed / sizeof(DWORD);

				for (unsigned int i = 0; i < count_processes; ++i)
				{
					if (processes_ids[i] != 0)
					{
						wchar_t process_name[1024] = L"<unknown>";

						HANDLE process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION |	PROCESS_VM_READ, FALSE, processes_ids[i]);
						if (process_handle)
						{
							HMODULE module = 0;
							DWORD cbNeeded = 0;

							if (::EnumProcessModules(process_handle, &module, sizeof(module), &cbNeeded))
							{
								if (::GetModuleFileNameEx(process_handle, module, process_name, sizeof(process_name)/sizeof(wchar_t)))
								{
									QString process_path = QString::fromUtf16((const ushort*) process_name);

									if (process_path.toLower() == path_installed.toLower())
									{
										::CloseHandle(process_handle);
										process_handle = 0;
										process_handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, processes_ids[i]);
										if (process_handle)
										{
											::TerminateProcess(process_handle, 0);
											if (::WaitForSingleObject(process_handle, 10000) != WAIT_TIMEOUT)
												res = true;

											::CloseHandle(process_handle);
											process_handle = 0;
										}
									}
								}
							}
							if (process_handle)
								::CloseHandle(process_handle);
						}
					}
				}	
			}
						
#endif //_WIN32

			return res;
		}

		installer::error terminate_process()
		{
            installer::error err;

#ifdef _WIN32
			HANDLE mutex = ::CreateSemaphore(NULL, 0, 1, get_crossprocess_mutex_name());
			if (ERROR_ALREADY_EXISTS == ::GetLastError())
			{
				const QString socket_name = get_crossprocess_pipe_name();
				const QString accept_socket_name = QString(get_crossprocess_pipe_name()) + crossprocess_pipe_name_postfix;

				if (!connect_to_process_exit_it(socket_name))
				{
					if (!abnormal_terminate())
						err = installer::error(errorcode::terminate_previous, QString("exit previous: ") + "can not terminate previous installation");
				}

			}
			
			if (mutex)
				::CloseHandle(mutex);
#endif //_WIN32

			return err;
		}
	}
}
