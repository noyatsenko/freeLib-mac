/*
  Copyright (c) 2011 - Tőkés Attila

  This file is part of SmtpClient for Qt.

  SmtpClient for Qt is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  SmtpClient for Qt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY.

  See the LICENSE file for more details.
*/

#include <QtCore>

#include "../../src/SmtpMime"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Create a MimeMessage

    MimeMessage message;

    EmailAddress sender("your_email_address@host.com", "Your Name");
    message.setSender(sender);

    EmailAddress to("recipient@host.com", "Recipient's Name");
    message.addRecipient(to);

    message.setSubject("SmtpClient for Qt - Example 3 - Html email with images");

    // Now we need to create a MimeHtml object for HTML content
    MimeHtml html;

    html.setHtml("<h1> Hello! </h1>"
                 "<h2> This is the first image </h2>"
                 "<img src='cid:image1' />"
                 "<h2> This is the second image </h2>"
                 "<img src='cid:image2' />");


    // Create a MimeInlineFile object for each image
    MimeInlineFile image1 (new QFile("image1.jpg"));

    // An unique content id must be setted
    image1.setContentId("image1");
    image1.setContentType("image/jpg");

    MimeInlineFile image2 (new QFile("image2.jpg"));
    image2.setContentId("image2");
    image2.setContentType("image/jpg");

    message.addPart(&html);
    message.addPart(&image1);
    message.addPart(&image2);

    // Now we can send the mail
    SmtpClient smtp("smtp.gmail.com", 465, SmtpClient::SslConnection);

    smtp.connectToHost();
    if (!smtp.waitForReadyConnected()) {
        qDebug() << "Failed to connect to host!" << endl;
        return -1;
    }

    smtp.login("your_email_address@host.com", "your_password");
    if (!smtp.waitForAuthenticated()) {
        qDebug() << "Failed to login!" << endl;
        return -2;
    }

    smtp.sendMail(message);
    if (!smtp.waitForMailSent()) {
        qDebug() << "Failed to send mail!" << endl;
        return -3;
    }
    smtp.quit();

}
