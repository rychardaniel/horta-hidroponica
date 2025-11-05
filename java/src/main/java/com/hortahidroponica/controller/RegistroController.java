package com.hortahidroponica.controller;

import com.hortahidroponica.model.Horta;
import com.hortahidroponica.model.Registro;
import jakarta.transaction.Transactional;
import jakarta.ws.rs.*;
import jakarta.ws.rs.core.MediaType;
import jakarta.ws.rs.core.Response;

@Path("/registro")
public class RegistroController {

    @POST
    @Path("/nome/{nome}")
    @Consumes(MediaType.APPLICATION_JSON)
    @Produces(MediaType.APPLICATION_JSON)
    @Transactional
    public Response InsereRegistro(@PathParam("nome") String nome, Registro registro) {
        Horta horta = Horta.find("nome", nome).firstResult();

        if (horta == null) {
            return Response.status(Response.Status.NOT_FOUND)
                    .entity("Horta com nome " + nome + " não encontrada.")
                    .build();
        }

        if (horta.parametro == null) {
            return Response.status(Response.Status.BAD_REQUEST)
                    .entity("Horta não possui parâmetro associado.")
                    .build();
        }

        Registro newRegistro = new Registro();
        newRegistro.horta = horta;
        newRegistro.parametro = horta.parametro;
        newRegistro.cultura = horta.parametro.cultura;
        newRegistro.nivelAgua = registro.nivelAgua;
        newRegistro.temperatura = registro.temperatura;
        newRegistro.ph = registro.ph;
        newRegistro.condutividade = registro.condutividade;
        newRegistro.dataRegistro = registro.dataRegistro;

        newRegistro.persist();

        return Response.ok().build();
    }

}
