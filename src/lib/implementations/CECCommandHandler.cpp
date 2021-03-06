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

#include "CECCommandHandler.h"
#include "../devices/CECBusDevice.h"
#include "../devices/CECAudioSystem.h"
#include "../devices/CECPlaybackDevice.h"
#include "../CECProcessor.h"

using namespace CEC;
using namespace std;

CCECCommandHandler::CCECCommandHandler(CCECBusDevice *busDevice)
{
  m_busDevice = busDevice;
}

bool CCECCommandHandler::HandleCommand(const cec_command &command)
{
  bool bHandled(true);

  CStdString strLog;
  strLog.Format(">> %s (%X) -> %s (%X): %s (%2X)", ToString(command.initiator), command.initiator, ToString(command.destination), command.destination, ToString(command.opcode), command.opcode);
  m_busDevice->AddLog(CEC_LOG_NOTICE, strLog);

  switch(command.opcode)
  {
  case CEC_OPCODE_REPORT_POWER_STATUS:
    HandleReportPowerStatus(command);
    break;
  case CEC_OPCODE_CEC_VERSION:
    HandleDeviceCecVersion(command);
    break;
  case CEC_OPCODE_SET_MENU_LANGUAGE:
    HandleSetMenuLanguage(command);
    break;
  case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
    HandleGivePhysicalAddress(command);
    break;
  case CEC_OPCODE_GIVE_OSD_NAME:
     HandleGiveOSDName(command);
    break;
  case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
    HandleGiveDeviceVendorId(command);
    break;
  case CEC_OPCODE_DEVICE_VENDOR_ID:
    HandleDeviceVendorId(command);
    break;
  case CEC_OPCODE_VENDOR_COMMAND_WITH_ID:
    HandleDeviceVendorCommandWithId(command);
    break;
  case CEC_OPCODE_GIVE_DECK_STATUS:
    HandleGiveDeckStatus(command);
    break;
  case CEC_OPCODE_DECK_CONTROL:
    HandleDeckControl(command);
    break;
  case CEC_OPCODE_MENU_REQUEST:
    HandleMenuRequest(command);
    break;
  case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
    HandleGiveDevicePowerStatus(command);
    break;
  case CEC_OPCODE_GET_CEC_VERSION:
    HandleGetCecVersion(command);
    break;
  case CEC_OPCODE_USER_CONTROL_PRESSED:
    HandleUserControlPressed(command);
    break;
  case CEC_OPCODE_USER_CONTROL_RELEASE:
    HandleUserControlRelease(command);
    break;
  case CEC_OPCODE_GIVE_AUDIO_STATUS:
    HandleGiveAudioStatus(command);
    break;
  case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
    HandleGiveSystemAudioModeStatus(command);
    break;
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
    HandleSetSystemAudioModeRequest(command);
    break;
  case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
    HandleRequestActiveSource(command);
    break;
  case CEC_OPCODE_SET_STREAM_PATH:
    HandleSetStreamPath(command);
    break;
  case CEC_OPCODE_ROUTING_CHANGE:
    HandleRoutingChange(command);
    break;
  case CEC_OPCODE_ROUTING_INFORMATION:
    HandleRoutingInformation(command);
    break;
  case CEC_OPCODE_STANDBY:
    HandleStandby(command);
    break;
  case CEC_OPCODE_ACTIVE_SOURCE:
    HandleActiveSource(command);
    break;
  case CEC_OPCODE_REPORT_PHYSICAL_ADDRESS:
    HandleReportPhysicalAddress(command);
    break;
  case CEC_OPCODE_REPORT_AUDIO_STATUS:
    HandleReportAudioStatus(command);
    break;
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:
    HandleSystemAudioStatus(command);
    break;
  case CEC_OPCODE_SET_OSD_NAME:
    HandleSetOSDName(command);
    break;
  case CEC_OPCODE_IMAGE_VIEW_ON:
    HandleImageViewOn(command);
    break;
  case CEC_OPCODE_TEXT_VIEW_ON:
    HandleTextViewOn(command);
    break;
  default:
    UnhandledCommand(command);
    bHandled = false;
    break;
  }

  if (command.destination == CECDEVICE_BROADCAST || m_busDevice->MyLogicalAddressContains(command.destination))
    m_busDevice->GetProcessor()->AddCommand(command);

  return bHandled;
}

bool CCECCommandHandler::HandleActiveSource(const cec_command &command)
{
  if (command.parameters.size == 2)
  {
    uint16_t iAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    return m_busDevice->GetProcessor()->SetStreamPath(iAddress);
  }

  return true;
}

bool CCECCommandHandler::HandleDeckControl(const cec_command &command)
{
  CCECBusDevice *device = GetDevice(command.destination);
  if (device && (device->GetType() == CEC_DEVICE_TYPE_PLAYBACK_DEVICE || device->GetType() == CEC_DEVICE_TYPE_RECORDING_DEVICE) && command.parameters.size > 0)
  {
    ((CCECPlaybackDevice *) device)->SetDeckControlMode((cec_deck_control_mode) command.parameters[0]);
    return true;
  }

  return false;
}

bool CCECCommandHandler::HandleDeviceCecVersion(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
      device->SetCecVersion((cec_version) command.parameters[0]);
  }

  return true;
}

bool CCECCommandHandler::HandleDeviceVendorCommandWithId(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
    m_busDevice->GetProcessor()->TransmitAbort(command.initiator, command.opcode, CEC_ABORT_REASON_REFUSED);

  return true;
}

bool CCECCommandHandler::HandleDeviceVendorId(const cec_command &command)
{
  SetVendorId(command);
  return true;
}

bool CCECCommandHandler::HandleGetCecVersion(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
      return device->TransmitCECVersion(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGiveAudioStatus(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
      return ((CCECAudioSystem *) device)->TransmitAudioStatus(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGiveDeckStatus(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && (device->GetType() == CEC_DEVICE_TYPE_PLAYBACK_DEVICE || device->GetType() == CEC_DEVICE_TYPE_RECORDING_DEVICE))
      return ((CCECPlaybackDevice *) device)->TransmitDeckStatus(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGiveDevicePowerStatus(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
      return device->TransmitPowerState(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGiveDeviceVendorId(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
      return device->TransmitVendorID(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGiveOSDName(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
      return device->TransmitOSDName(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleGivePhysicalAddress(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device)
      return device->TransmitPhysicalAddress();
  }

  return false;
}

bool CCECCommandHandler::HandleGiveSystemAudioModeStatus(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device && device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
      return ((CCECAudioSystem *) device)->TransmitSystemAudioModeStatus(command.initiator);
  }

  return false;
}

bool CCECCommandHandler::HandleImageViewOn(const cec_command &command)
{
  m_busDevice->GetProcessor()->SetActiveSource(command.initiator);
  return true;
}

bool CCECCommandHandler::HandleMenuRequest(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
  {
    if (command.parameters[0] == CEC_MENU_REQUEST_TYPE_QUERY)
    {
      CCECBusDevice *device = GetDevice(command.destination);
      if (device)
        return device->TransmitMenuState(command.initiator);
    }
  }

  return false;
}

bool CCECCommandHandler::HandleReportAudioStatus(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device && device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
    {
      ((CCECAudioSystem *)device)->SetAudioStatus(command.parameters[0]);
      return true;
    }
  }
  return false;
}

bool CCECCommandHandler::HandleReportPhysicalAddress(const cec_command &command)
{
  if (command.parameters.size == 3)
  {
    uint16_t iNewAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    SetPhysicalAddress(command.initiator, iNewAddress);
  }
  return true;
}

bool CCECCommandHandler::HandleReportPowerStatus(const cec_command &command)
{
  if (command.parameters.size == 1)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
      device->SetPowerStatus((cec_power_status) command.parameters[0]);
  }
  return true;
}

bool CCECCommandHandler::HandleRequestActiveSource(const cec_command &command)
{
  CStdString strLog;
  strLog.Format(">> %i requests active source", (uint8_t) command.initiator);
  m_busDevice->AddLog(CEC_LOG_DEBUG, strLog.c_str());

  vector<CCECBusDevice *> devices;
  for (int iDevicePtr = (int)GetMyDevices(devices)-1; iDevicePtr >=0; iDevicePtr--)
    devices[iDevicePtr]->TransmitActiveSource();

  return true;
}

bool CCECCommandHandler::HandleRoutingChange(const cec_command &command)
{
  if (command.parameters.size == 4)
  {
    uint16_t iOldAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    uint16_t iNewAddress = ((uint16_t)command.parameters[2] << 8) | ((uint16_t)command.parameters[3]);

    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
      device->SetStreamPath(iNewAddress, iOldAddress);
  }
  return true;
}

bool CCECCommandHandler::HandleRoutingInformation(const cec_command &command)
{
  if (command.parameters.size == 2)
  {
    uint16_t iNewAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    m_busDevice->GetProcessor()->SetStreamPath(iNewAddress);
  }

  return false;
}

bool CCECCommandHandler::HandleSetMenuLanguage(const cec_command &command)
{
  if (command.parameters.size == 3)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      cec_menu_language language;
      language.device = command.initiator;
      for (uint8_t iPtr = 0; iPtr < 4; iPtr++)
        language.language[iPtr] = command.parameters[iPtr];
      language.language[3] = 0;
      device->SetMenuLanguage(language);
      return true;
    }
  }
  return false;
}

bool CCECCommandHandler::HandleSetOSDName(const cec_command &command)
{
  if (command.parameters.size > 0)
  {
    CCECBusDevice *device = GetDevice(command.initiator);
    if (device)
    {
      char buf[1024];
      for (uint8_t iPtr = 0; iPtr < command.parameters.size; iPtr++)
        buf[iPtr] = (char)command.parameters[iPtr];
      buf[command.parameters.size] = 0;

      CStdString strName(buf);
      device->SetOSDName(strName);

      return true;
    }
  }
  return false;
}

bool CCECCommandHandler::HandleSetStreamPath(const cec_command &command)
{
  if (command.parameters.size >= 2)
  {
    uint16_t iStreamAddress = ((uint16_t)command.parameters[0] << 8) | ((uint16_t)command.parameters[1]);
    CStdString strLog;
    strLog.Format(">> %i sets stream path to physical address %04x", command.initiator, iStreamAddress);
    m_busDevice->AddLog(CEC_LOG_DEBUG, strLog.c_str());

    if (m_busDevice->GetProcessor()->SetStreamPath(iStreamAddress))
    {
      CCECBusDevice *device = GetDeviceByPhysicalAddress(iStreamAddress);
      if (device)
      {
        return device->TransmitActiveSource() &&
            device->TransmitMenuState(command.initiator);
      }
    }
  }
  return false;
}

bool CCECCommandHandler::HandleSetSystemAudioModeRequest(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination) && command.parameters.size >= 1)
  {
    CCECBusDevice *device = GetDevice(command.destination);
    if (device&& device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
      return ((CCECAudioSystem *) device)->SetSystemAudioMode(command);
  }
  return false;
}

bool CCECCommandHandler::HandleStandby(const cec_command &command)
{
  CCECBusDevice *device = GetDevice(command.initiator);
  if (device)
    device->SetPowerStatus(CEC_POWER_STATUS_STANDBY);

  return true;
}

bool CCECCommandHandler::HandleSystemAudioStatus(const cec_command &command)
{
  CCECBusDevice *device = GetDevice(command.initiator);
  if (device && device->GetType() == CEC_DEVICE_TYPE_AUDIO_SYSTEM)
  {
    ((CCECAudioSystem *)device)->SetSystemAudioMode(command);
    return true;
  }

  return false;
}

bool CCECCommandHandler::HandleTextViewOn(const cec_command &command)
{
  m_busDevice->GetProcessor()->SetActiveSource(command.initiator);
  return true;
}

bool CCECCommandHandler::HandleUserControlPressed(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination) && command.parameters.size > 0)
  {
    m_busDevice->GetProcessor()->AddKey();

    if (command.parameters[0] <= CEC_USER_CONTROL_CODE_MAX)
    {
      CStdString strLog;
      strLog.Format("key pressed: %x", command.parameters[0]);
      m_busDevice->AddLog(CEC_LOG_DEBUG, strLog.c_str());

      if (command.parameters[0] == CEC_USER_CONTROL_CODE_POWER ||
          command.parameters[0] == CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION)
      {
        CCECBusDevice *device = GetDevice(command.destination);
        if (device)
          device->SetPowerStatus(CEC_POWER_STATUS_ON);
      }

      m_busDevice->GetProcessor()->SetCurrentButton((cec_user_control_code) command.parameters[0]);
      return true;
    }
  }
  return false;
}

bool CCECCommandHandler::HandleUserControlRelease(const cec_command &command)
{
  if (m_busDevice->MyLogicalAddressContains(command.destination))
    m_busDevice->GetProcessor()->AddKey();

  return true;
}

void CCECCommandHandler::UnhandledCommand(const cec_command &command)
{
  CStdString strLog;
  strLog.Format("unhandled command with opcode %02x from address %d", command.opcode, command.initiator);
  m_busDevice->AddLog(CEC_LOG_DEBUG, strLog);
}

unsigned int CCECCommandHandler::GetMyDevices(vector<CCECBusDevice *> &devices) const
{
  unsigned int iReturn(0);

  cec_logical_addresses addresses = m_busDevice->GetProcessor()->GetLogicalAddresses();
  for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
  {
    if (addresses[iPtr])
    {
      devices.push_back(GetDevice((cec_logical_address) iPtr));
      ++iReturn;
    }
  }

  return iReturn;
}

CCECBusDevice *CCECCommandHandler::GetDevice(cec_logical_address iLogicalAddress) const
{
  CCECBusDevice *device = NULL;

  if (iLogicalAddress >= CECDEVICE_TV && iLogicalAddress <= CECDEVICE_BROADCAST)
    device = m_busDevice->GetProcessor()->m_busDevices[iLogicalAddress];

  return device;
}

CCECBusDevice *CCECCommandHandler::GetDeviceByPhysicalAddress(uint16_t iPhysicalAddress) const
{
  return m_busDevice->GetProcessor()->GetDeviceByPhysicalAddress(iPhysicalAddress);
}

CCECBusDevice *CCECCommandHandler::GetDeviceByType(cec_device_type type) const
{
  return m_busDevice->GetProcessor()->GetDeviceByType(type);
}

void CCECCommandHandler::SetVendorId(const cec_command &command)
{
  if (command.parameters.size < 3)
  {
    m_busDevice->AddLog(CEC_LOG_WARNING, "invalid vendor ID received");
    return;
  }

  uint64_t iVendorId = ((uint64_t)command.parameters[0] << 16) +
                       ((uint64_t)command.parameters[1] << 8) +
                        (uint64_t)command.parameters[2];

  CCECBusDevice *device = GetDevice((cec_logical_address) command.initiator);
  if (device)
    device->SetVendorId(iVendorId);
}

void CCECCommandHandler::SetPhysicalAddress(cec_logical_address iAddress, uint16_t iNewAddress)
{
  if (!m_busDevice->MyLogicalAddressContains(iAddress))
  {
    bool bOurAddress(m_busDevice->GetProcessor()->GetPhysicalAddress() == iNewAddress);
    GetDevice(iAddress)->SetPhysicalAddress(iNewAddress);
    if (bOurAddress)
    {
      /* another device reported the same physical address as ours
       * since we don't have physical address detection yet, we'll just use the
       * given address, increased by 0x100 for now */
      m_busDevice->GetProcessor()->SetPhysicalAddress(iNewAddress + 0x100);
    }
  }
}

const char *CCECCommandHandler::ToString(const cec_menu_state state)
{
  switch (state)
  {
  case CEC_MENU_STATE_ACTIVATED:
    return "activated";
  case CEC_MENU_STATE_DEACTIVATED:
    return "deactivated";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_version version)
{
  switch (version)
  {
  case CEC_VERSION_1_2:
    return "1.2";
  case CEC_VERSION_1_2A:
    return "1.2a";
  case CEC_VERSION_1_3:
    return "1.3";
  case CEC_VERSION_1_3A:
    return "1.3a";
  case CEC_VERSION_1_4:
    return "1.4";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_power_status status)
{
  switch (status)
  {
  case CEC_POWER_STATUS_ON:
    return "on";
  case CEC_POWER_STATUS_STANDBY:
    return "standby";
  case CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY:
    return "in transition from on to standby";
  case CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON:
    return "in transition from standby to on";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_logical_address address)
{
  switch(address)
  {
  case CECDEVICE_AUDIOSYSTEM:
    return "Audio";
  case CECDEVICE_BROADCAST:
    return "Broadcast";
  case CECDEVICE_FREEUSE:
    return "Free use";
  case CECDEVICE_PLAYBACKDEVICE1:
    return "Playback 1";
  case CECDEVICE_PLAYBACKDEVICE2:
    return "Playback 2";
  case CECDEVICE_PLAYBACKDEVICE3:
    return "Playback 3";
  case CECDEVICE_RECORDINGDEVICE1:
    return "Recorder 1";
  case CECDEVICE_RECORDINGDEVICE2:
    return "Recorder 2";
  case CECDEVICE_RECORDINGDEVICE3:
    return "Recorder 3";
  case CECDEVICE_RESERVED1:
    return "Reserved 1";
  case CECDEVICE_RESERVED2:
    return "Reserved 2";
  case CECDEVICE_TUNER1:
    return "Tuner 1";
  case CECDEVICE_TUNER2:
    return "Tuner 2";
  case CECDEVICE_TUNER3:
    return "Tuner 3";
  case CECDEVICE_TUNER4:
    return "Tuner 4";
  case CECDEVICE_TV:
    return "TV";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_deck_control_mode mode)
{
  switch (mode)
  {
  case CEC_DECK_CONTROL_MODE_SKIP_FORWARD_WIND:
    return "skip forward wind";
  case CEC_DECK_CONTROL_MODE_EJECT:
    return "eject";
  case CEC_DECK_CONTROL_MODE_SKIP_REVERSE_REWIND:
    return "reverse rewind";
  case CEC_DECK_CONTROL_MODE_STOP:
    return "stop";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_deck_info status)
{
  switch (status)
  {
  case CEC_DECK_INFO_PLAY:
    return "play";
  case CEC_DECK_INFO_RECORD:
    return "record";
  case CEC_DECK_INFO_PLAY_REVERSE:
    return "play reverse";
  case CEC_DECK_INFO_STILL:
    return "still";
  case CEC_DECK_INFO_SLOW:
    return "slow";
  case CEC_DECK_INFO_SLOW_REVERSE:
    return "slow reverse";
  case CEC_DECK_INFO_FAST_FORWARD:
    return "fast forward";
  case CEC_DECK_INFO_FAST_REVERSE:
    return "fast reverse";
  case CEC_DECK_INFO_NO_MEDIA:
    return "no media";
  case CEC_DECK_INFO_STOP:
    return "stop";
  case CEC_DECK_INFO_SKIP_FORWARD_WIND:
    return "info skip forward wind";
  case CEC_DECK_INFO_SKIP_REVERSE_REWIND:
    return "info skip reverse rewind";
  case CEC_DECK_INFO_INDEX_SEARCH_FORWARD:
    return "info index search forward";
  case CEC_DECK_INFO_INDEX_SEARCH_REVERSE:
    return "info index search reverse";
  case CEC_DECK_INFO_OTHER_STATUS:
    return "other";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_opcode opcode)
{
  switch (opcode)
  {
  case CEC_OPCODE_ACTIVE_SOURCE:
    return "active source";
  case CEC_OPCODE_IMAGE_VIEW_ON:
    return "image view on";
  case CEC_OPCODE_TEXT_VIEW_ON:
    return "text view on";
  case CEC_OPCODE_INACTIVE_SOURCE:
    return "inactive source";
  case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
    return "request active source";
  case CEC_OPCODE_ROUTING_CHANGE:
    return "routing change";
  case CEC_OPCODE_ROUTING_INFORMATION:
    return "routing information";
  case CEC_OPCODE_SET_STREAM_PATH:
    return "set stream path";
  case CEC_OPCODE_STANDBY:
    return "standby";
  case CEC_OPCODE_RECORD_OFF:
    return "record off";
  case CEC_OPCODE_RECORD_ON:
    return "record on";
  case CEC_OPCODE_RECORD_STATUS:
    return "record status";
  case CEC_OPCODE_RECORD_TV_SCREEN:
    return "record tv screen";
  case CEC_OPCODE_CLEAR_ANALOGUE_TIMER:
    return "clear analogue timer";
  case CEC_OPCODE_CLEAR_DIGITAL_TIMER:
    return "clear digital timer";
  case CEC_OPCODE_CLEAR_EXTERNAL_TIMER:
    return "clear external timer";
  case CEC_OPCODE_SET_ANALOGUE_TIMER:
    return "set analogue timer";
  case CEC_OPCODE_SET_DIGITAL_TIMER:
    return "set digital timer";
  case CEC_OPCODE_SET_EXTERNAL_TIMER:
    return "set external timer";
  case CEC_OPCODE_SET_TIMER_PROGRAM_TITLE:
    return "set timer program title";
  case CEC_OPCODE_TIMER_CLEARED_STATUS:
    return "timer cleared status";
  case CEC_OPCODE_TIMER_STATUS:
    return "timer status";
  case CEC_OPCODE_CEC_VERSION:
    return "cec version";
  case CEC_OPCODE_GET_CEC_VERSION:
    return "get cec version";
  case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
    return "give physical address";
  case CEC_OPCODE_GET_MENU_LANGUAGE:
    return "get menu language";
  case CEC_OPCODE_REPORT_PHYSICAL_ADDRESS:
    return "report physical address";
  case CEC_OPCODE_SET_MENU_LANGUAGE:
    return "set menu language";
  case CEC_OPCODE_DECK_CONTROL:
    return "deck control";
  case CEC_OPCODE_DECK_STATUS:
    return "deck status";
  case CEC_OPCODE_GIVE_DECK_STATUS:
    return "give deck status";
  case CEC_OPCODE_PLAY:
    return "play";
  case CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS:
    return "give tuner status";
  case CEC_OPCODE_SELECT_ANALOGUE_SERVICE:
    return "select analogue service";
  case CEC_OPCODE_SELECT_DIGITAL_SERVICE:
    return "set digital service";
  case CEC_OPCODE_TUNER_DEVICE_STATUS:
    return "tuner device status";
  case CEC_OPCODE_TUNER_STEP_DECREMENT:
    return "tuner step decrement";
  case CEC_OPCODE_TUNER_STEP_INCREMENT:
    return "tuner step increment";
  case CEC_OPCODE_DEVICE_VENDOR_ID:
    return "device vendor id";
  case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
    return "give device vendor id";
  case CEC_OPCODE_VENDOR_COMMAND:
    return "vendor command";
  case CEC_OPCODE_VENDOR_COMMAND_WITH_ID:
    return "vendor command with id";
  case CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN:
    return "vendor remote button down";
  case CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP:
    return "vendor remote button up";
  case CEC_OPCODE_SET_OSD_STRING:
    return "set osd string";
  case CEC_OPCODE_GIVE_OSD_NAME:
    return "give osd name";
  case CEC_OPCODE_SET_OSD_NAME:
    return "set osd name";
  case CEC_OPCODE_MENU_REQUEST:
    return "menu request";
  case CEC_OPCODE_MENU_STATUS:
    return "menu status";
  case CEC_OPCODE_USER_CONTROL_PRESSED:
    return "user control pressed";
  case CEC_OPCODE_USER_CONTROL_RELEASE:
    return "user control release";
  case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
    return "give device power status";
  case CEC_OPCODE_REPORT_POWER_STATUS:
    return "report power status";
  case CEC_OPCODE_FEATURE_ABORT:
    return "feature abort";
  case CEC_OPCODE_ABORT:
    return "abort";
  case CEC_OPCODE_GIVE_AUDIO_STATUS:
    return "give audio status";
  case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
    return "give audio mode status";
  case CEC_OPCODE_REPORT_AUDIO_STATUS:
    return "report audio status";
  case CEC_OPCODE_SET_SYSTEM_AUDIO_MODE:
    return "set system audio mode";
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
    return "system audio mode request";
  case CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:
    return "system audio mode status";
  case CEC_OPCODE_SET_AUDIO_RATE:
    return "set audio rate";
  default:
    return "UNKNOWN";
  }
}

const char *CCECCommandHandler::ToString(const cec_system_audio_status mode)
{
  switch(mode)
  {
  case CEC_SYSTEM_AUDIO_STATUS_ON:
    return "on";
  case CEC_SYSTEM_AUDIO_STATUS_OFF:
    return "off";
  default:
    return "unknown";
  }
}

const char *CCECCommandHandler::ToString(const cec_audio_status status)
{
  // TODO this is a mask
  return "TODO";
}

const char *CCECCommandHandler::ToString(const cec_vendor_id vendor)
{
  switch (vendor)
  {
  case CEC_VENDOR_SAMSUNG:
    return "Samsung";
  case CEC_VENDOR_LG:
    return "LG";
  case CEC_VENDOR_PANASONIC:
    return "Panasonic";
  case CEC_VENDOR_PIONEER:
    return "Pioneer";
  case CEC_VENDOR_ONKYO:
    return "Onkyo";
  default:
    return "Unknown";
  }
}
