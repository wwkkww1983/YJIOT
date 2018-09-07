package com.yjyz.iot.share.action;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import com.yjyz.iot.comm.ConstParm;
import com.yjyz.iot.device.entity.DeviceInfo;
import com.yjyz.iot.exam.entity.DevExamDetail;
import com.yjyz.iot.exam.service.IDevExamService;
import com.yjyz.iot.notice.service.INoticeService;
import com.yjyz.iot.ota.entity.OTAFileInfo;
import com.yjyz.iot.share.dto.M26RetDataDto;
import com.yjyz.iot.share.service.IM26DeviceService;

@CrossOrigin
@RestController
@RequestMapping("/m26")
public class M26Controller {
	private static Log log = LogFactory.getLog(M26Controller.class);
	@Autowired
	private IM26DeviceService m26DeviceService;
	@Autowired
	private IDevExamService devExamService;
	@Autowired
	INoticeService noticeService;

	@RequestMapping(value = "/activate")
	public M26RetDataDto activate(@RequestParam(value = "PID") String productId,
			@RequestParam(value = "CCID") String deviceMac, @RequestParam(value = "PW") String devicePw,
			HttpServletRequest request) {
		log.debug("Function activate execute.");
		log.debug("PID:" + productId);
		log.debug("CCID:" + deviceMac);
		log.debug("PW:" + devicePw);

		DeviceInfo deviceInfo = new DeviceInfo();
		deviceInfo.setProductId(productId);
		deviceInfo.setDeviceMac(deviceMac);
		deviceInfo.setDevicePw(devicePw);

		M26RetDataDto retData = new M26RetDataDto();
		try {
			String ip = request.getHeader("X-Forwarded-For");
			if (ip == null || ip.length() == 0 || "unknown".equalsIgnoreCase(ip)) {
				ip = request.getHeader("Proxy-Client-IP");
			}
			if (ip == null || ip.length() == 0 || "unknown".equalsIgnoreCase(ip)) {
				ip = request.getHeader("WL-Proxy-Client-IP");
			}
			if (ip == null || ip.length() == 0 || "unknown".equalsIgnoreCase(ip)) {
				ip = request.getHeader("HTTP_CLIENT_IP");
			}
			if (ip == null || ip.length() == 0 || "unknown".equalsIgnoreCase(ip)) {
				ip = request.getHeader("HTTP_X_FORWARDED_FOR");
			}
			if (ip == null || ip.length() == 0 || "unknown".equalsIgnoreCase(ip)) {
				ip = request.getRemoteAddr();
			}
			// log.error("mxchipsn: " + ip);
			ip = ip.split(",")[0];
			deviceInfo.setRegIp(ip);
			deviceInfo.setIsActivate(true);
			deviceInfo = this.m26DeviceService.activate(deviceInfo);
			retData.setC(ConstParm.SUCESS_CODE);
			Map<String, Object> map = new HashMap<String, Object>();
			map.put("DID", deviceInfo.getDeviceId());
			retData.setD(map);
		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_ACTIVATE);
		}
		return retData;
	}

	@RequestMapping(value = "/otaCheck")
	public M26RetDataDto otaCheck(@RequestParam(value = "DID") String deviceId,
			@RequestParam(value = "MN") String moduleName, @RequestParam(value = "FW") String firmware,
			@RequestParam(value = "IV") String iotVersion) {
		log.debug("Function otaCheck execute.");
		log.debug("DID:" + deviceId);
		log.debug("MN:" + moduleName);
		log.debug("FW:" + firmware);
		log.debug("IV:" + iotVersion);

		DeviceInfo deviceInfo = new DeviceInfo();
		deviceInfo.setDeviceId(deviceId);
		deviceInfo.setModuleName(moduleName);
		deviceInfo.setFirmware(firmware);
		deviceInfo.setFirmwareType("ATIOT");
		deviceInfo.setIotVersion(iotVersion);
		M26RetDataDto retData = new M26RetDataDto();
		try {
			OTAFileInfo otaFile = this.m26DeviceService.otaCheck(deviceInfo);
			if (otaFile != null) {
				retData.setC(ConstParm.SUCESS_CODE);
				Map<String, Object> map = new HashMap<String, Object>();
				Map<String, Object> file = new HashMap<String, Object>();
				file.put("fileurl", otaFile.getFileurl());
				file.put("md5", otaFile.getMd5());
				file.put("size", otaFile.getSize());
				file.put("firmWare", otaFile.getFirmware());

				map.put("FF", ConstParm.DICT_SHARE_M26_DATA);
				map.put("FD", file);
				retData.setD(map);
			} else {
				retData.setC(ConstParm.SUCESS_CODE);
				Map<String, Object> map = new HashMap<String, Object>();
				map.put("FF", ConstParm.DICT_SHARE_M26_NODATA);
				retData.setD(map);
			}
		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_OTACHECK);
		}
		return retData;
	}

	@RequestMapping(value = "/UD")
	public M26RetDataDto uploadData(@RequestParam(value = "DID") String deviceId,
			@RequestParam(value = "SF") Integer saveFlag,
			@RequestParam(value = "NT", required = false, defaultValue = "0") Integer noticeType,
			@RequestParam(value = "PN", required = false, defaultValue = "0") Integer pushNo,
			@RequestParam(value = "SD", required = false) String jsonData) {
		log.debug("Function UD execute.");
		log.debug("DID:" + deviceId);
		log.debug("SF:" + saveFlag);
		log.debug("SD" + jsonData);

		M26RetDataDto retData = new M26RetDataDto();
		try {
			boolean isSave = saveFlag != null;
			isSave = this.m26DeviceService.uploadData(deviceId, isSave, jsonData);
			if (isSave) {
				retData.setC(ConstParm.SUCESS_CODE);
			} else {
				retData.setC(ConstParm.ERR_SAVE);
			}
			// 发送通知
			if (noticeType != 0) {
				noticeService.noticeSend(deviceId, noticeType,pushNo);
			}
		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_UPLOADDATA);
		}
		return retData;
	}

	@RequestMapping(value = "/SS")
	public M26RetDataDto syncStatus(@RequestParam(value = "DID") String deviceId,
			@RequestParam(value = "SD", required = false) String jsonData) {
		log.debug("Function SS execute.");
		log.debug("DID:" + deviceId);
		log.debug("SD" + jsonData);

		M26RetDataDto retData = new M26RetDataDto();
		try {
			Map<String, Object> data = this.m26DeviceService.syncStatus(deviceId, jsonData);
			retData.setC(ConstParm.SUCESS_CODE);
			if (data != null) {
				retData.setD(data);
			}

		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_SYNCSTATUS);
		}
		return retData;
	}

	@RequestMapping(value = "/CC")
	public M26RetDataDto chipsConfirm(@RequestParam(value = "DID") String deviceId,
			@RequestParam(value = "ORN") String orderNo) {
		log.debug("Function CC execute.");
		log.debug("DID:" + deviceId);
		log.debug("ORN" + orderNo);

		M26RetDataDto retData = new M26RetDataDto();
		try {
			boolean isOK = this.m26DeviceService.chipsConfirm(deviceId, orderNo);
			if (isOK) {
				retData.setC(ConstParm.SUCESS_CODE);
			} else {
				retData.setC(ConstParm.ERR_SHARE_M26_CHIPSCONFIRM);
			}
		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_CHIPSCONFIRM);
		}
		return retData;
	}

	@RequestMapping(value = "/SEI")
	public M26RetDataDto saveExamItem(@RequestParam(value = "EID") String examId,
			@RequestParam(value = "ED") String jsonData) {
		log.debug("Function saveExamItem execute.");
		log.debug("EID:" + examId);
		log.debug("ED" + jsonData);
		M26RetDataDto retData = new M26RetDataDto();
		try {
			String[] ers = jsonData.split(",");
			List<DevExamDetail> examItems = new ArrayList<DevExamDetail>();
			for (int i = 0; i < ers.length; i++) {
				String temp = ers[i];
				DevExamDetail detial = new DevExamDetail();
				detial.setExamItem(temp.split(":")[0]);
				detial.setResult(BigDecimal.valueOf(Double.parseDouble(temp.split(":")[1])));
				examItems.add(detial);
			}
			boolean isOk = this.devExamService.saveExamItem(examId, examItems);
			if (isOk) {
				retData.setC(ConstParm.SUCESS_CODE);
			} else {
				retData.setC(ConstParm.ERR_SHARE_M26_CHIPSCONFIRM);
			}
		} catch (Exception e) {
			log.error(e);
			retData.setC(ConstParm.ERR_SHARE_M26_CHIPSCONFIRM);
		}
		return retData;
	}

}
