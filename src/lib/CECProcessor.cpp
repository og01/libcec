/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "CECProcessor.h"

#include "AdapterCommunication.h"
#include "devices/CECBusDevice.h"
#include "devices/CECAudioSystem.h"
#include "devices/CECPlaybackDevice.h"
#include "devices/CECRecordingDevice.h"
#include "devices/CECTuner.h"
#include "devices/CECTV.h"
#include "implementations/CECCommandHandler.h"
#include "LibCEC.h"
#include "util/StdString.h"
#include "platform/timeutils.h"

using namespace CEC;
using namespace std;

CCECProcessor::CCECProcessor(CLibCEC *controller, CAdapterCommunication *serComm, const char *strDeviceName, cec_logical_address iLogicalAddress /* = CECDEVICE_PLAYBACKDEVICE1 */, uint16_t iPhysicalAddress /* = CEC_DEFAULT_PHYSICAL_ADDRESS*/) :
    m_bStarted(false),
    m_iHDMIPort(CEC_DEFAULT_HDMI_PORT),
    m_strDeviceName(strDeviceName),
    m_communication(serComm),
    m_controller(controller),
    m_bMonitor(false)
{
  m_logicalAddresses.Clear();
  m_logicalAddresses.Set(iLogicalAddress);
  m_types.clear();
  for (int iPtr = 0; iPtr <= 16; iPtr++)
    m_busDevices[iPtr] = new CCECBusDevice(this, (cec_logical_address) iPtr, iPtr == iLogicalAddress ? iPhysicalAddress : 0);
}

CCECProcessor::CCECProcessor(CLibCEC *controller, CAdapterCommunication *serComm, const char *strDeviceName, const cec_device_type_list &types) :
    m_bStarted(false),
    m_iHDMIPort(CEC_DEFAULT_HDMI_PORT),
    m_strDeviceName(strDeviceName),
    m_types(types),
    m_communication(serComm),
    m_controller(controller),
    m_bMonitor(false)
{
  m_logicalAddresses.Clear();
  for (int iPtr = 0; iPtr < 16; iPtr++)
  {
    switch(iPtr)
    {
    case CECDEVICE_AUDIOSYSTEM:
      m_busDevices[iPtr] = new CCECAudioSystem(this, (cec_logical_address) iPtr, 0xFFFF);
      break;
    case CECDEVICE_PLAYBACKDEVICE1:
    case CECDEVICE_PLAYBACKDEVICE2:
    case CECDEVICE_PLAYBACKDEVICE3:
      m_busDevices[iPtr] = new CCECPlaybackDevice(this, (cec_logical_address) iPtr, 0xFFFF);
      break;
    case CECDEVICE_RECORDINGDEVICE1:
    case CECDEVICE_RECORDINGDEVICE2:
    case CECDEVICE_RECORDINGDEVICE3:
      m_busDevices[iPtr] = new CCECRecordingDevice(this, (cec_logical_address) iPtr, 0xFFFF);
      break;
    case CECDEVICE_TUNER1:
    case CECDEVICE_TUNER2:
    case CECDEVICE_TUNER3:
    case CECDEVICE_TUNER4:
      m_busDevices[iPtr] = new CCECTuner(this, (cec_logical_address) iPtr, 0xFFFF);
      break;
    case CECDEVICE_TV:
      m_busDevices[iPtr] = new CCECTV(this, (cec_logical_address) iPtr, 0);
      break;
    default:
      m_busDevices[iPtr] = new CCECBusDevice(this, (cec_logical_address) iPtr, 0xFFFF);
      break;
    }
  }
}

CCECProcessor::~CCECProcessor(void)
{
  m_startCondition.Broadcast();
  StopThread();
  m_communication = NULL;
  m_controller = NULL;
  for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
    delete m_busDevices[iPtr];
}

bool CCECProcessor::Start(void)
{
  CLockObject lock(&m_mutex);
  if (!m_communication || !m_communication->IsOpen())
  {
    m_controller->AddLog(CEC_LOG_ERROR, "connection is closed");
    return false;
  }

  if (CreateThread())
  {
    if (!m_startCondition.Wait(&m_mutex) || !m_bStarted)
    {
      m_controller->AddLog(CEC_LOG_ERROR, "could not create a processor thread");
      return false;
    }
    return true;
  }
  else
    m_controller->AddLog(CEC_LOG_ERROR, "could not create a processor thread");

  return false;
}

bool CCECProcessor::TryLogicalAddress(cec_logical_address address)
{
  if (m_busDevices[address]->TryLogicalAddress())
  {
    /* only set our OSD name and active source for the primary device */
    if (m_logicalAddresses.IsEmpty())
    {
      m_busDevices[address]->m_strDeviceName = m_strDeviceName;
      m_busDevices[address]->m_bActiveSource = true;
    }
    m_logicalAddresses.Set(address);
    return true;
  }

  return false;
}

bool CCECProcessor::FindLogicalAddressRecordingDevice(void)
{
  AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'recording device'");
  return TryLogicalAddress(CECDEVICE_RECORDINGDEVICE1) ||
      TryLogicalAddress(CECDEVICE_RECORDINGDEVICE2) ||
      TryLogicalAddress(CECDEVICE_RECORDINGDEVICE3);
}

bool CCECProcessor::FindLogicalAddressTuner(void)
{
  AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'tuner'");
  return TryLogicalAddress(CECDEVICE_TUNER1) ||
      TryLogicalAddress(CECDEVICE_TUNER2) ||
      TryLogicalAddress(CECDEVICE_TUNER3) ||
      TryLogicalAddress(CECDEVICE_TUNER4);
}

bool CCECProcessor::FindLogicalAddressPlaybackDevice(void)
{
  AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'playback device'");
  return TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE1) ||
      TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE2) ||
      TryLogicalAddress(CECDEVICE_PLAYBACKDEVICE3);
}

bool CCECProcessor::FindLogicalAddressAudioSystem(void)
{
  AddLog(CEC_LOG_DEBUG, "detecting logical address for type 'audio'");
  return TryLogicalAddress(CECDEVICE_AUDIOSYSTEM);
}

bool CCECProcessor::FindLogicalAddresses(void)
{
  bool bReturn(true);
  m_logicalAddresses.Clear();
  CStdString strLog;

  for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
  {
    if (m_types.types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
      continue;

    strLog.Format("%s - device %d: type %d", __FUNCTION__, iPtr, m_types.types[iPtr]);
    AddLog(CEC_LOG_DEBUG, strLog);

    if (m_types.types[iPtr] == CEC_DEVICE_TYPE_RECORDING_DEVICE)
      bReturn &= FindLogicalAddressRecordingDevice();
    if (m_types.types[iPtr] == CEC_DEVICE_TYPE_TUNER)
      bReturn &= FindLogicalAddressTuner();
    if (m_types.types[iPtr] == CEC_DEVICE_TYPE_PLAYBACK_DEVICE)
      bReturn &= FindLogicalAddressPlaybackDevice();
    if (m_types.types[iPtr] == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
      bReturn &= FindLogicalAddressAudioSystem();
  }

  return bReturn;
}

void *CCECProcessor::Process(void)
{
  bool                  bParseFrame(false);
  cec_command           command;
  CCECAdapterMessage    msg;

  {
    if (m_logicalAddresses.IsEmpty() && !FindLogicalAddresses())
    {
      CLockObject lock(&m_mutex);
      m_controller->AddLog(CEC_LOG_ERROR, "could not detect our logical addresses");
      m_startCondition.Signal();
      return NULL;
    }

    SetAckMask(m_logicalAddresses.AckMask());

    {
      CLockObject lock(&m_mutex);
      m_bStarted = true;
      lock.Leave();

      SetHDMIPort(m_iHDMIPort);

      lock.Lock();
      m_controller->AddLog(CEC_LOG_DEBUG, "processor thread started");
      m_startCondition.Signal();
    }
  }

  while (!IsStopped())
  {
    command.Clear();
    msg.clear();

    {
      CLockObject lock(&m_mutex);
      if (m_commandBuffer.Pop(command))
      {
        bParseFrame = true;
      }
      else if (m_communication->IsOpen() && m_communication->Read(msg, 50))
      {
        m_controller->AddLog(msg.is_error() ? CEC_LOG_WARNING : CEC_LOG_DEBUG, msg.ToString());
        if ((bParseFrame = (ParseMessage(msg) && !IsStopped())) == true)
          command = m_currentframe;
      }
    }

    if (bParseFrame)
      ParseCommand(command);
    bParseFrame = false;

    Sleep(5);

    m_controller->CheckKeypressTimeout();

    for (unsigned int iDevicePtr = 0; iDevicePtr < 16; iDevicePtr++)
    {
      if (!m_logicalAddresses[iDevicePtr])
        m_busDevices[iDevicePtr]->PollVendorId();
    }

    Sleep(5);
  }

  return NULL;
}

bool CCECProcessor::SetActiveSource(cec_device_type type /* = CEC_DEVICE_TYPE_RESERVED */)
{
  bool bReturn(false);

  if (!IsRunning())
    return bReturn;

  cec_logical_address addr = m_logicalAddresses.primary;

  if (type != CEC_DEVICE_TYPE_RESERVED)
  {
    for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
    {
      if (m_logicalAddresses[iPtr] && m_busDevices[iPtr]->m_type == type)
      {
        addr = (cec_logical_address) iPtr;
        break;
      }
    }
  }

  return SetStreamPath(m_busDevices[addr]->GetPhysicalAddress(false)) &&
      m_busDevices[addr]->TransmitActiveSource();
}

bool CCECProcessor::SetActiveSource(cec_logical_address iAddress)
{
  return SetStreamPath(m_busDevices[iAddress]->GetPhysicalAddress(false));
}

bool CCECProcessor::SetActiveView(void)
{
  return SetActiveSource(m_types.IsEmpty() ? CEC_DEVICE_TYPE_RESERVED : m_types[0]);
}

bool CCECProcessor::SetDeckControlMode(cec_deck_control_mode mode, bool bSendUpdate /* = true */)
{
  bool bReturn(false);

  CCECBusDevice *device = GetDeviceByType(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
  if (device)
  {
    ((CCECPlaybackDevice *) device)->SetDeckControlMode(mode);
    if (bSendUpdate)
      ((CCECPlaybackDevice *) device)->TransmitDeckStatus(CECDEVICE_TV);
    bReturn = true;
  }

  return bReturn;
}

bool CCECProcessor::SetDeckInfo(cec_deck_info info, bool bSendUpdate /* = true */)
{
  bool bReturn(false);

  CCECBusDevice *device = GetDeviceByType(CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
  if (device)
  {
    ((CCECPlaybackDevice *) device)->SetDeckStatus(info);
    if (bSendUpdate)
      ((CCECPlaybackDevice *) device)->TransmitDeckStatus(CECDEVICE_TV);
    bReturn = true;
  }

  return bReturn;
}

bool CCECProcessor::SetHDMIPort(uint8_t iPort)
{
  bool bReturn(false);

  CStdString strLog;
  strLog.Format("setting HDMI port to %d", iPort);
  AddLog(CEC_LOG_DEBUG, strLog);

  m_iHDMIPort = iPort;
  if (!m_bStarted)
    return true;

  uint16_t iPhysicalAddress(0);
  int iPos = 3;
  while(!bReturn && iPos >= 0)
  {
    iPhysicalAddress += ((uint16_t)iPort * (0x1 << iPos*4));
    strLog.Format("checking physical address %4x", iPhysicalAddress);
    AddLog(CEC_LOG_DEBUG, strLog);
    if (CheckPhysicalAddress(iPhysicalAddress))
    {
      strLog.Format("physical address %4x is in use", iPhysicalAddress);
      AddLog(CEC_LOG_DEBUG, strLog);
      iPos--;
    }
    else
    {
      SetPhysicalAddress(iPhysicalAddress);
      bReturn = true;
    }
  }

  return bReturn;
}

bool CCECProcessor::CheckPhysicalAddress(uint16_t iPhysicalAddress)
{
  for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
  {
    if (m_busDevices[iPtr]->GetPhysicalAddress(false) == iPhysicalAddress)
      return true;
  }
  return false;
}

bool CCECProcessor::SetStreamPath(uint16_t iStreamPath)
{
  bool bReturn(false);

  CCECBusDevice *device = GetDeviceByPhysicalAddress(iStreamPath);
  if (device)
  {
    device->SetActiveDevice();
    bReturn = true;
  }

  return bReturn;
}

bool CCECProcessor::SetInactiveView(void)
{
  if (!IsRunning())
    return false;

  if (!m_logicalAddresses.IsEmpty() && m_busDevices[m_logicalAddresses.primary])
    return m_busDevices[m_logicalAddresses.primary]->TransmitInactiveView();
  return false;
}

void CCECProcessor::LogOutput(const cec_command &data)
{
  CStdString strTx;
  strTx.Format("<< %02x", ((uint8_t)data.initiator << 4) + (uint8_t)data.destination);
  if (data.opcode_set)
      strTx.AppendFormat(":%02x", (uint8_t)data.opcode);

  for (uint8_t iPtr = 0; iPtr < data.parameters.size; iPtr++)
    strTx.AppendFormat(":%02x", data.parameters[iPtr]);
  m_controller->AddLog(CEC_LOG_TRAFFIC, strTx.c_str());
}

bool CCECProcessor::SetLogicalAddress(cec_logical_address iLogicalAddress)
{
  if (m_logicalAddresses.primary != iLogicalAddress)
  {
    CStdString strLog;
    strLog.Format("<< setting primary logical address to %1x", iLogicalAddress);
    m_controller->AddLog(CEC_LOG_NOTICE, strLog.c_str());
    m_logicalAddresses.primary = iLogicalAddress;
    m_logicalAddresses.Set(iLogicalAddress);
    return SetAckMask(m_logicalAddresses.AckMask());
  }

  return true;
}

bool CCECProcessor::SetMenuState(cec_menu_state state, bool bSendUpdate /* = true */)
{
  for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
  {
    if (m_logicalAddresses[iPtr])
      m_busDevices[iPtr]->SetMenuState(state);
  }

  if (bSendUpdate)
    m_busDevices[m_logicalAddresses.primary]->TransmitMenuState(CECDEVICE_TV);

  return true;
}

bool CCECProcessor::SetPhysicalAddress(uint16_t iPhysicalAddress)
{
  if (!m_logicalAddresses.IsEmpty())
  {
    for (uint8_t iPtr = 0; iPtr < 15; iPtr++)
      if (m_logicalAddresses[iPtr])
        m_busDevices[iPtr]->SetPhysicalAddress(iPhysicalAddress);
    return SetActiveView();
  }
  return false;
}

bool CCECProcessor::SwitchMonitoring(bool bEnable)
{
  CStdString strLog;
  strLog.Format("== %s monitoring mode ==", bEnable ? "enabling" : "disabling");
  m_controller->AddLog(CEC_LOG_NOTICE, strLog.c_str());

  m_bMonitor = bEnable;
  if (bEnable)
    return SetAckMask(0);
  else
    return SetAckMask(m_logicalAddresses.AckMask());
}

bool CCECProcessor::PollDevice(cec_logical_address iAddress)
{
  if (iAddress != CECDEVICE_UNKNOWN && m_busDevices[iAddress])
    return m_busDevices[m_logicalAddresses.primary]->TransmitPoll(iAddress);
  return false;
}

uint8_t CCECProcessor::VolumeUp(void)
{
  uint8_t status = 0;
  if (IsActiveDevice(CECDEVICE_AUDIOSYSTEM))
    status = ((CCECAudioSystem *)m_busDevices[CECDEVICE_AUDIOSYSTEM])->VolumeUp();

  return status;
}

uint8_t CCECProcessor::VolumeDown(void)
{
  uint8_t status = 0;
  if (IsActiveDevice(CECDEVICE_AUDIOSYSTEM))
    status = ((CCECAudioSystem *)m_busDevices[CECDEVICE_AUDIOSYSTEM])->VolumeDown();

  return status;
}

uint8_t CCECProcessor::MuteAudio(void)
{
  uint8_t status = 0;
  if (IsActiveDevice(CECDEVICE_AUDIOSYSTEM))
    status = ((CCECAudioSystem *)m_busDevices[CECDEVICE_AUDIOSYSTEM])->MuteAudio();

  return status;
}

CCECBusDevice *CCECProcessor::GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress, bool bRefresh /* = false */) const
{
  if (m_busDevices[m_logicalAddresses.primary]->GetPhysicalAddress(false) == iPhysicalAddress)
    return m_busDevices[m_logicalAddresses.primary];

  CCECBusDevice *device = NULL;
  for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
  {
    if (m_busDevices[iPtr]->GetPhysicalAddress(bRefresh) == iPhysicalAddress)
    {
      device = m_busDevices[iPtr];
      break;
    }
  }

  return device;
}

CCECBusDevice *CCECProcessor::GetDeviceByType(cec_device_type type) const
{
  CCECBusDevice *device = NULL;

  for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
  {
    if (m_busDevices[iPtr]->m_type == type)
    {
      device = m_busDevices[iPtr];
      break;
    }
  }

  return device;
}

cec_version CCECProcessor::GetDeviceCecVersion(cec_logical_address iAddress)
{
  return m_busDevices[iAddress]->GetCecVersion();
}

bool CCECProcessor::GetDeviceMenuLanguage(cec_logical_address iAddress, cec_menu_language *language)
{
  if (m_busDevices[iAddress])
  {
    *language = m_busDevices[iAddress]->GetMenuLanguage();
    return (strcmp(language->language, "???") != 0);
  }
  return false;
}

uint64_t CCECProcessor::GetDeviceVendorId(cec_logical_address iAddress)
{
  if (m_busDevices[iAddress])
    return m_busDevices[iAddress]->GetVendorId();
  return false;
}

cec_power_status CCECProcessor::GetDevicePowerStatus(cec_logical_address iAddress)
{
  if (m_busDevices[iAddress])
    return m_busDevices[iAddress]->GetPowerStatus();
  return CEC_POWER_STATUS_UNKNOWN;
}

bool CCECProcessor::Transmit(const cec_command &data)
{
  bool bReturn(false);
  LogOutput(data);

  CCECAdapterMessage *output = new CCECAdapterMessage(data);
  bReturn = Transmit(output);
  delete output;

  return bReturn;
}

bool CCECProcessor::Transmit(CCECAdapterMessage *output)
{
  bool bReturn(false);
  CLockObject lock(&m_mutex);
  {
    CLockObject msgLock(&output->mutex);
    if (!m_communication || !m_communication->Write(output))
      return bReturn;
    else
    {
      output->condition.Wait(&output->mutex);
      if (output->state != ADAPTER_MESSAGE_STATE_SENT)
      {
        m_controller->AddLog(CEC_LOG_ERROR, "command was not sent");
        return bReturn;
      }
    }

    if (output->transmit_timeout > 0)
    {
      if ((bReturn = WaitForTransmitSucceeded(output->size(), output->transmit_timeout)) == false)
        m_controller->AddLog(CEC_LOG_DEBUG, "did not receive ack");
    }
    else
      bReturn = true;
  }

  return bReturn;
}

void CCECProcessor::TransmitAbort(cec_logical_address address, cec_opcode opcode, cec_abort_reason reason /* = CEC_ABORT_REASON_UNRECOGNIZED_OPCODE */)
{
  m_controller->AddLog(CEC_LOG_DEBUG, "<< transmitting abort message");

  cec_command command;
  // TODO
  cec_command::Format(command, m_logicalAddresses.primary, address, CEC_OPCODE_FEATURE_ABORT);
  command.parameters.PushBack((uint8_t)opcode);
  command.parameters.PushBack((uint8_t)reason);

  Transmit(command);
}

bool CCECProcessor::WaitForTransmitSucceeded(uint8_t iLength, uint32_t iTimeout /* = 1000 */)
{
  bool bError(false);
  bool bTransmitSucceeded(false);
  uint8_t iPacketsLeft(iLength / 4);

  int64_t iNow = GetTimeMs();
  int64_t iTargetTime = iNow + (uint64_t) iTimeout;

  while (!bTransmitSucceeded && !bError && (iTimeout == 0 || iNow < iTargetTime))
  {
    CCECAdapterMessage msg;

    if (!m_communication->Read(msg, iTimeout > 0 ? (int32_t)(iTargetTime - iNow) : 1000))
    {
      iNow = GetTimeMs();
      continue;
    }

    if ((bError = msg.is_error()) == false)
    {
      m_controller->AddLog(bError ? CEC_LOG_WARNING : CEC_LOG_DEBUG, msg.ToString());

      switch(msg.message())
      {
      case MSGCODE_COMMAND_ACCEPTED:
        if (iPacketsLeft > 0)
          iPacketsLeft--;
        break;
      case MSGCODE_TRANSMIT_SUCCEEDED:
        bTransmitSucceeded = (iPacketsLeft == 0);
        bError = !bTransmitSucceeded;
        break;
      default:
        if (ParseMessage(msg))
          m_commandBuffer.Push(m_currentframe);
      }

      iNow = GetTimeMs();
    }
  }

  return bTransmitSucceeded && !bError;
}

bool CCECProcessor::ParseMessage(const CCECAdapterMessage &msg)
{
  bool bEom = false;

  if (msg.empty())
    return bEom;

  switch(msg.message())
  {
  case MSGCODE_FRAME_START:
    {
      m_currentframe.Clear();
      if (msg.size() >= 2)
      {
        m_currentframe.initiator   = msg.initiator();
        m_currentframe.destination = msg.destination();
        m_currentframe.ack         = msg.ack();
        m_currentframe.eom         = msg.eom();
      }
    }
    break;
  case MSGCODE_FRAME_DATA:
    {
      if (msg.size() >= 2)
      {
        m_currentframe.PushBack(msg[1]);
        m_currentframe.eom = msg.eom();
      }
      bEom = msg.eom();
    }
    break;
  default:
    break;
  }

  return bEom;
}

void CCECProcessor::ParseCommand(cec_command &command)
{
  CStdString dataStr;
  dataStr.Format(">> %1x%1x:%02x", command.initiator, command.destination, command.opcode);
  for (uint8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
    dataStr.AppendFormat(":%02x", (unsigned int)command.parameters[iPtr]);
  m_controller->AddLog(CEC_LOG_TRAFFIC, dataStr.c_str());

  if (!m_bMonitor && command.initiator >= CECDEVICE_TV && command.initiator <= CECDEVICE_BROADCAST)
    m_busDevices[(uint8_t)command.initiator]->HandleCommand(command);
}

cec_logical_addresses CCECProcessor::GetActiveDevices(void)
{
  cec_logical_addresses addresses;
  for (unsigned int iPtr = 0; iPtr < 15; iPtr++)
  {
    if (m_busDevices[iPtr]->GetStatus() == CEC_DEVICE_STATUS_PRESENT)
      addresses.Set((cec_logical_address) iPtr);
  }
  return addresses;
}

bool CCECProcessor::IsActiveDevice(cec_logical_address address)
{
  return m_busDevices[address]->GetStatus() == CEC_DEVICE_STATUS_PRESENT;
}

bool CCECProcessor::IsActiveDeviceType(cec_device_type type)
{
  for (unsigned int iPtr = 0; iPtr < 15; iPtr++)
  {
    if (m_busDevices[iPtr]->GetType() == type && m_busDevices[iPtr]->GetStatus() == CEC_DEVICE_STATUS_PRESENT)
      return true;
  }

  return false;
}

uint16_t CCECProcessor::GetPhysicalAddress(void) const
{
  if (!m_logicalAddresses.IsEmpty() && m_busDevices[m_logicalAddresses.primary])
    return m_busDevices[m_logicalAddresses.primary]->GetPhysicalAddress(false);
  return false;
}

void CCECProcessor::SetCurrentButton(cec_user_control_code iButtonCode)
{
  m_controller->SetCurrentButton(iButtonCode);
}

void CCECProcessor::AddCommand(const cec_command &command)
{
  m_controller->AddCommand(command);
}

void CCECProcessor::AddKey(cec_keypress &key)
{
  m_controller->AddKey(key);
}

void CCECProcessor::AddKey(void)
{
  m_controller->AddKey();
}

void CCECProcessor::AddLog(cec_log_level level, const CStdString &strMessage)
{
  m_controller->AddLog(level, strMessage);
}

bool CCECProcessor::SetAckMask(uint16_t iMask)
{
  bool bReturn(false);
  CStdString strLog;
  strLog.Format("setting ackmask to %2x", iMask);
  m_controller->AddLog(CEC_LOG_DEBUG, strLog.c_str());

  CCECAdapterMessage *output = new CCECAdapterMessage;

  output->push_back(MSGSTART);
  output->push_escaped(MSGCODE_SET_ACK_MASK);
  output->push_escaped(iMask >> 8);
  output->push_escaped((uint8_t)iMask);
  output->push_back(MSGEND);

  if ((bReturn = Transmit(output)) == false)
    m_controller->AddLog(CEC_LOG_ERROR, "could not set the ackmask");

  delete output;

  return bReturn;
}
