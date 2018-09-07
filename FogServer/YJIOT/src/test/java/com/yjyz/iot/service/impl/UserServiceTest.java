package com.yjyz.iot.service.impl;

import org.apache.log4j.Logger;
import org.junit.Test;
import org.springframework.beans.factory.annotation.Autowired;

import com.yjyz.iot.SpringTestCase;
import com.yjyz.iot.example.servcie.impl.UserServiceImpl;
import com.yjyz.iot.example.dto.User;

public class UserServiceTest extends SpringTestCase {
	@Autowired
	private UserServiceImpl userService;
	Logger logger = Logger.getLogger(UserServiceTest.class);

	@Test
	public void selectUserByIdTest() {
		User user = userService.getUserById(1);
		System.out.println(user.getUserName());
	}

}
